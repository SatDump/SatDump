/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "logger.h"

#if 1

#include "dsp/device/dev.h"
#include "init.h"

int main(int argc, char *argv[])
{
    initLogger();

    logger->set_level(slog::LOG_ERROR);
    satdump::initSatdump();
    completeLoggerInit();
    logger->set_level(slog::LOG_TRACE);

    auto devs = satdump::ndsp::getDeviceList();

    for (auto &d : devs)
    {
        auto i = satdump::ndsp::getDeviceInstanceFromInfo(d);
        d.params = i->get_cfg_list();
        logger->debug("\n" + nlohmann::json(d).dump(4) + "\n");
    }
}

#else
#include <nng/nng.h>
#include <nng/protocol/bus0/bus.h>
#include <nng/protocol/pair0/pair.h>
#include <nng/supplemental/http/http.h>
#include <nng/supplemental/util/platform.h>

#include "common/dsp/fft/fft_pan.h"
#include "common/dsp_source_sink/dsp_sample_source.h"
#include "init.h"

#include <unistd.h>

// HTTP Handler for stats
void http_handle(nng_aio *aio)
{
    std::string jsonstr = "{\"api\": true}";

    nng_http_res *res;
    nng_http_res_alloc(&res);
    nng_http_res_copy_data(res, jsonstr.c_str(), jsonstr.size());
    nng_http_res_set_header(res, "Content-Type", "application/json; charset=utf-8");
    nng_aio_set_output(aio, 0, res);
    nng_aio_finish(aio, 0);
}

// HTTP Handler for commands
void http_handleP(nng_aio *aio)
{
    std::string jsonstr = "{\"api\": true}";

    nng_http_req *msg = (nng_http_req *)nng_aio_get_input(aio, 0);

    void *ptr;
    size_t ptrl;
    nng_http_req_get_data(msg, &ptr, &ptrl);
    logger->info("Got : %s", (char *)ptr);

    nng_http_res *res;
    nng_http_res_alloc(&res);
    nng_http_res_copy_data(res, jsonstr.c_str(), jsonstr.size());
    nng_http_res_set_header(res, "Content-Type", "application/json; charset=utf-8");
    nng_aio_set_output(aio, 0, res);
    nng_aio_finish(aio, 0);
}

int main(int argc, char *argv[])
{
    initLogger();

    logger->set_level(slog::LOG_OFF);
    satdump::initSatdump();
    completeLoggerInit();
    logger->set_level(slog::LOG_TRACE);

    dsp::registerAllSources();

    ///////////////////////

    std::shared_ptr<dsp::DSPSampleSource> dsp_source;
    {
        auto all_sources = dsp::getAllAvailableSources();
        for (auto &s : all_sources)
        {
            logger->trace(s.name);
            if (s.source_type == "rtlsdr")
                dsp_source = dsp::getSourceFromDescriptor(s);
        }
    }

    dsp_source->open();
    dsp_source->set_samplerate(2.4e6);
    dsp_source->set_frequency(431.8e6);
    dsp_source->start();

    std::shared_ptr<dsp::FFTPanBlock> fftPan = std::make_shared<dsp::FFTPanBlock>(dsp_source->output_stream);
    fftPan->set_fft_settings(65536, dsp_source->get_samplerate(), 30);
    fftPan->avg_num = 1;

    ////////////////////////

    std::string http_server_url = "http://0.0.0.0:8080";

    nng_http_server *http_server;
    nng_url *url;
    {

        nng_url_parse(&url, http_server_url.c_str());
        nng_http_server_hold(&http_server, url);
    }

    nng_http_handler *handler_api;
    {
        nng_http_handler_alloc(&handler_api, "/api", http_handle);
        nng_http_handler_set_method(handler_api, "GET");
        nng_http_server_add_handler(http_server, handler_api);
    }

    nng_http_handler *handler_apiP;
    {
        nng_http_handler_alloc(&handler_api, "/apip", http_handleP);
        nng_http_handler_set_method(handler_api, "POST");
        nng_http_server_add_handler(http_server, handler_api);
    }

    nng_socket socket;

    {

        int rv = 0;
        if (rv = nng_bus0_open(&socket); rv != 0)
        {
            printf("pair open error\n");
        }

        if (rv = nng_listen(socket, "ws://0.0.0.0:8080/ws", nullptr, 0); rv != 0)
        {
            printf("server listen error\n");
        }
    }

    nng_url_free(url);

    nng_http_server_start(http_server);

    fftPan->on_fft = [&](float *b)
    {
        std::vector<uint8_t> send_buf(8 + 65536);
        float *min = (float *)&send_buf[0];
        float *max = (float *)&send_buf[4];
        uint8_t *dat = &send_buf[8];

        *min = 1e6;
        *max = -1e6;

        for (int i = 0; i < 65536; i++)
        {
            if (*min > b[i])
                *min = b[i];
            if (*max < b[i])
                *max = b[i];
        }

        for (int i = 0; i < 65536; i++)
        {
            float val = (b[i] - *min) / (*max - *min);
            dat[i] = val * 255;
        }

        nng_send(socket, send_buf.data(), send_buf.size(), NNG_FLAG_NONBLOCK);
    };
    fftPan->start();

    while (1)
    {
        /*char *buf = nullptr;
        size_t size;

        int rv = 0;
        printf("WAIT\n");
        if (rv = nng_recv(socket, &buf, &size, NNG_FLAG_ALLOC); rv != 0)
        {
            printf("recv error: %s\n", nng_strerror(rv));
        }

        printf("server get with client: %s\n", buf);

        std::string testStr = "Server Reply!\n\r";
        //  int rv = 0;
        if (rv = nng_send(socket, (void *)testStr.c_str(), testStr.size(), NULL); rv != 0)
        {
            printf("send error: %s\n", nng_strerror(rv));
        }

        printf("SEND\n");
*/
        sleep(1);
    }

    nng_http_server_stop(http_server);
    nng_http_server_release(http_server);
}
#endif
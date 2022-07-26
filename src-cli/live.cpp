#include "live.h"
#include "common/dsp_sample_source/dsp_sample_source.h"
#include "core/live_pipeline.h"
#include <signal.h>
#include "logger.h"
#include "init.h"
#include "common/cli_utils.h"
#include <nng/nng.h>
#include <nng/supplemental/http/http.h>
#include <nng/supplemental/util/platform.h>

// Catch CTRL+C to exit live properly!
bool live_should_exit = false;
void sig_handler_live(int signo)
{
    if (signo == SIGINT)
        live_should_exit = true;
}

// Webserver for stats
namespace webserver
{
    nng_http_server *http_server;
    nng_url *url;
    nng_http_handler *handler;

    satdump::LivePipeline *live_pipeline;
    bool is_active = false;

    // HTTP Handler for stats
    void http_handle(nng_aio *aio)
    {
        std::string jsonstr = live_pipeline->getModulesStats().dump(4);

        nng_http_res *res;
        nng_http_res_alloc(&res);
        nng_http_res_copy_data(res, jsonstr.c_str(), jsonstr.size());
        nng_http_res_set_header(res, "Content-Type", "application/json; charset=utf-8");
        nng_aio_set_output(aio, 0, res);
        nng_aio_finish(aio, 0);
    }

    void start(std::string http_server_url)
    {
        http_server_url = "http://" + http_server_url;
        nng_url_parse(&url, http_server_url.c_str());
        nng_http_server_hold(&http_server, url);
        nng_http_handler_alloc(&handler, url->u_path, http_handle);
        nng_http_handler_set_method(handler, "GET");
        nng_http_server_add_handler(http_server, handler);
        nng_http_server_start(http_server);
        nng_url_free(url);
        is_active = true;
    }

    void stop()
    {
        if (is_active)
        {
            nng_http_server_stop(http_server);
            nng_http_server_release(http_server);
        }
    }
};

int main_live(int argc, char *argv[])
{
    if (argc < 5) // Check overall command
    {
        logger->error("Usage : " + std::string(argv[0]) + " live [pipeline_id] [output_file_or_directory] [additional options as required]");
        logger->error("Extra options (examples. Any parameter used in modules or sources can be used here) :");
        logger->error(" --samplerate [baseband_samplerate] --baseband_format [f32/s16/s8/u8] --dc_block --iq_swap");
        logger->error(" --source [airspy/rtlsdr/etc] --gain 20 --bias");
        logger->error("As well as --timeout in seconds");
        logger->error("Sample command :");
        logger->error("./satdump live metop_ahrpt metop_output_directory --source airspy --samplerate 6e6 --frequency 1701.3e6 --general_gain 18 --bias --timeout 780");
        return 1;
    }

    // Init SatDump
    satdump::initSatdump();

    std::string downlink_pipeline = argv[2];
    std::string output_file = argv[3];

    // Parse flags
    nlohmann::json parameters = parse_common_flags(argc - 4, &argv[4]);

    if (parameters.contains("client"))
    {
        logger->info("Starting in client mode!");

        // Create output dir
        if (!std::filesystem::exists(output_file))
            std::filesystem::create_directories(output_file);

        // Get pipeline
        std::optional<satdump::Pipeline> pipeline = satdump::getPipelineFromName(downlink_pipeline);

        if (!pipeline.has_value())
        {
            logger->critical("Pipeline " + downlink_pipeline + " does not exist!");
            return 1;
        }

        // Init pipeline
        std::unique_ptr<satdump::LivePipeline> live_pipeline = std::make_unique<satdump::LivePipeline>(pipeline.value(), parameters, output_file);

        ctpl::thread_pool live_thread_pool(8);

        // Attempt to start the source and pipeline
        try
        {
            live_pipeline->start_client(live_thread_pool);
        }
        catch (std::exception &e)
        {
            logger->error("Fatal error running pipeline/device : " + std::string(e.what()));
            return 1;
        }

        // If requested, boot up webserver
        if (parameters.contains("http_server"))
        {
            std::string http_addr = parameters["http_server"].get<std::string>();
            webserver::live_pipeline = live_pipeline.get();
            logger->info("Start webserver on {:s}", http_addr.c_str());
            webserver::start(http_addr);
        }

        // Attach signal
        signal(SIGINT, sig_handler_live);

        // Now, we wait
        while (1)
        {
            if (live_should_exit)
            {
                logger->warn("SIGINT Received. Stopping.");
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Stop cleanly
        live_pipeline->stop();
    }
    else
    {
        uint64_t samplerate;
        uint64_t frequency;
        uint64_t timeout;
        std::string handler_id;

        try
        {
            samplerate = parameters["samplerate"].get<uint64_t>();
            frequency = parameters["frequency"].get<uint64_t>();
            timeout = parameters.contains("timeout") ? parameters["timeout"].get<uint64_t>() : 0;
            handler_id = parameters["source"].get<std::string>();
        }
        catch (std::exception &e)
        {
            logger->error("Error parsing arguments! {:s}", e.what());
            return 1;
        }

        // Create output dir
        if (!std::filesystem::exists(output_file))
            std::filesystem::create_directories(output_file);

        // Get all sources
        dsp::registerAllSources();
        std::vector<dsp::SourceDescriptor> source_tr = dsp::getAllAvailableSources();
        dsp::SourceDescriptor selected_src;

        for (dsp::SourceDescriptor src : source_tr)
            logger->debug("Device " + src.name);

        // Try to find it and check it's usable
        bool src_found = false;
        for (dsp::SourceDescriptor src : source_tr)
        {
            if (handler_id == src.source_type)
            {
                selected_src = src;
                src_found = true;
            }
        }

        if (!src_found)
        {
            logger->error("Could not find a handler for source type : {:s}!", handler_id.c_str());
            return 1;
        }

        // Init source
        std::shared_ptr<dsp::DSPSampleSource> source_ptr = getSourceFromDescriptor(selected_src);
        source_ptr->open();
        source_ptr->set_frequency(frequency);
        source_ptr->set_samplerate(samplerate);
        source_ptr->set_settings(parameters);

        // Get pipeline
        std::optional<satdump::Pipeline> pipeline = satdump::getPipelineFromName(downlink_pipeline);

        if (!pipeline.has_value())
        {
            logger->critical("Pipeline " + downlink_pipeline + " does not exist!");
            return 1;
        }

        // Init pipeline
        parameters["baseband_format"] = "f32";
        parameters["buffer_size"] = STREAM_BUFFER_SIZE; // This is required, as we WILL go over the (usually) default 8192 size
        std::unique_ptr<satdump::LivePipeline> live_pipeline = std::make_unique<satdump::LivePipeline>(pipeline.value(), parameters, output_file);

        ctpl::thread_pool live_thread_pool(8);

        bool server_mode = parameters.contains("server_address") || parameters.contains("server_port");

        // Attempt to start the source and pipeline
        try
        {
            source_ptr->start();
            live_pipeline->start(source_ptr->output_stream, live_thread_pool, server_mode);
        }
        catch (std::exception &e)
        {
            logger->error("Fatal error running pipeline/device : " + std::string(e.what()));
            return 1;
        }

        // If requested, boot up webserver
        if (parameters.contains("http_server"))
        {
            std::string http_addr = parameters["http_server"].get<std::string>();
            webserver::live_pipeline = live_pipeline.get();
            logger->info("Start webserver on {:s}", http_addr.c_str());
            webserver::start(http_addr);
        }

        // Attach signal
        signal(SIGINT, sig_handler_live);

        // Now, we wait
        uint64_t start_time = time(0);
        while (1)
        {
            if (timeout > 0)
            {
                uint64_t elapsed_time = time(0) - start_time;
                if (elapsed_time >= timeout)
                {
                    logger->warn("Timeout is over! ({:d}s >= {:d}s) Stopping.", elapsed_time, timeout);
                    break;
                }
            }

            if (live_should_exit)
            {
                logger->warn("SIGINT Received. Stopping.");
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Stop cleanly
        source_ptr->stop();
        live_pipeline->stop();
    }

    if (parameters.contains("http_server"))
        webserver::stop();

    return 0;
}
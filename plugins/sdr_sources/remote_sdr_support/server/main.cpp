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
#include "init.h"
#include "remote.h"
#include "udp_discovery.h"

#include <unistd.h>

#include "common/dsp_source_sink/dsp_sample_source.h"
#include "common/dsp_source_sink/dsp_sample_sink.h"

#include "tcp_test.h"

#include "common/rimgui.h"

#include "iq_pkt.h"

// The TCP Server
TCPServer *tcp_server;

// Currently open source & statuses
std::mutex source_mtx;
std::shared_ptr<dsp::DSPSampleSource> current_sample_source;
uint64_t last_samplerate = 0;
bool source_is_open = false;
bool source_is_started = false;

std::mutex gui_feedback_mtx;
RImGui::RImGui gui_local;
std::vector<RImGui::UiElem> last_draw_feedback;

void sourceGuiThread()
{
    RImGui::is_local = false;
    RImGui::current_instance = &gui_local;

    std::vector<uint8_t> buffer_tx;

    while (1)
    {
        source_mtx.lock();
        if (source_is_open)
        {
            gui_feedback_mtx.lock();
            if (last_draw_feedback.size() > 0)
            {
                logger->info("FeedBack %d", last_draw_feedback.size());
                RImGui::set_feedback(last_draw_feedback);
                last_draw_feedback.clear();
            }
            gui_feedback_mtx.unlock();

            current_sample_source->drawControlUI();

            auto render_elems = RImGui::end_frame();

            if (render_elems.size() > 0)
            {
                logger->info("DrawElems %d", render_elems.size());
                gui_feedback_mtx.lock();
                buffer_tx.resize(65535);
                int len = RImGui::encode_vec(buffer_tx.data(), render_elems);
                buffer_tx.resize(len);
                sendPacketWithVector(tcp_server, dsp::remote::PKT_TYPE_GUI, buffer_tx);
                gui_feedback_mtx.unlock();
            }

            if (last_samplerate != current_sample_source->get_samplerate())
            {
                std::vector<uint8_t> pkt(8);
                *((uint64_t *)&pkt[0]) = last_samplerate = current_sample_source->get_samplerate();
                sendPacketWithVector(tcp_server, dsp::remote::PKT_TYPE_SAMPLERATEFBK, pkt);
            }
        }
        source_mtx.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

std::mutex source_stream_mtx;
bool source_should_stream = false;
void sourceStreamThread()
{
    uint8_t *buffer_tx = new uint8_t[3000000];
    float *mag_buffer = new float[dsp::STREAM_BUFFER_SIZE];
    while (1)
    {
        source_stream_mtx.lock();
        if (source_should_stream)
        {
            {
                int nsamples = current_sample_source->output_stream->read();

                if (nsamples <= 0)
                    continue;

                // if (nsamples < 4096)
                {
                    int pktlen = 3;
                    buffer_tx[0] = dsp::remote::PKT_TYPE_IQ;
                    // buffer_tx[1] = nsamples >> 8;
                    // buffer_tx[2] = nsamples & 0xFF;
                    // memcpy(&buffer_tx[3], current_sample_source->output_stream->readBuf, nsamples * sizeof(complex_t));
                    pktlen += remote_sdr::encode_iq_pkt(&buffer_tx[1], current_sample_source->output_stream->readBuf, mag_buffer, nsamples, 8);

                    tcp_server->swrite(buffer_tx, pktlen);
                }
                /* else
                 {
                     int sent_samples = 0;
                     while ((nsamples - sent_samples) > 0)
                     {
                         int to_send = (nsamples - sent_samples) > 4096 ? 4096 : (nsamples - sent_samples);

                         int pktlen = 3;
                         buffer_tx[0] = dsp::remote::PKT_TYPE_IQ;
                         buffer_tx[1] = to_send >> 8;
                         buffer_tx[2] = to_send & 0xFF;
                         // memcpy(&buffer_tx[3], current_sample_source->output_stream->readBuf + sent_samples, to_send * sizeof(complex_t));
                         pktlen += ziq2::ziq2_write_iq_pkt(&buffer_tx[3], current_sample_source->output_stream->readBuf + sent_samples, mag_buffer, to_send, 8, false);

                         tcp_server->swrite(buffer_tx, pktlen);
                         // logger->info("NSAMPLES %d %d", to_send, to_send * sizeof(complex_t) + 3);

                         sent_samples += to_send;
                     }
                 }*/

                // logger->trace(nsamples);
                current_sample_source->output_stream->flush();
            }
        }
        source_stream_mtx.unlock();

        if (!source_should_stream)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    delete[] buffer_tx;
}

void tcp_rx_handler(uint8_t *buffer, int len)
{
    int pkt_type = buffer[0];

    if (pkt_type == dsp::remote::PKT_TYPE_PING)
    { // Simply reply
        sendPacketWithVector(tcp_server, dsp::remote::PKT_TYPE_PING);
        logger->debug("Ping!");
    }

    if (pkt_type == dsp::remote::PKT_TYPE_SOURCELIST)
    {
        std::vector<dsp::SourceDescriptor> sources = dsp::getAllAvailableSources(true);
        std::vector<dsp::SourceDescriptor> sources_final;

        logger->trace("Found devices (sources) :");
        for (dsp::SourceDescriptor src : sources)
        {
            logger->trace("- " + src.name);

            if (src.source_type == "file" ||
                src.source_type == "plutosdr" ||
                src.source_type == "rtltcp" ||
                src.source_type == "sdrpp_server" ||
                src.source_type == "spyserver" ||
                src.source_type == "udp_source")
                continue;

            sources_final.push_back(src);
        }

        // TODO SIMPLIFY TO BINARY?
        sendPacketWithVector(tcp_server, dsp::remote::PKT_TYPE_SOURCELIST, nlohmann::json::to_cbor(nlohmann::json(sources_final)));
    }

    if (pkt_type == dsp::remote::PKT_TYPE_SOURCEOPEN)
    {
        source_mtx.lock();
        if (source_is_open)
        {
            current_sample_source->close();
            current_sample_source.reset();
            logger->info("Source closed!");
        }

        dsp::SourceDescriptor source = nlohmann::json::from_cbor(std::vector<uint8_t>(&buffer[1], &buffer[len]));

        logger->info("Opening " + source.name);

        std::vector<dsp::SourceDescriptor> sources = dsp::getAllAvailableSources(true);
        for (dsp::SourceDescriptor src : sources)
        {
            if (src.name == source.name && src.unique_id == source.unique_id)
            {
                current_sample_source = dsp::getSourceFromDescriptor(src);
                current_sample_source->open();

                source_is_open = true;
                last_samplerate = 0;

                logger->info("Source opened!");
            }
        }
        source_mtx.unlock();
    }

    if (pkt_type == dsp::remote::PKT_TYPE_SOURCECLOSE)
    {
        source_mtx.lock();
        if (source_is_open)
        {
            if (current_sample_source)
            {
                current_sample_source->close();
                current_sample_source.reset();
                source_is_open = false;
                logger->info("Source closed!");
            }
        }
        source_mtx.unlock();
    }

    if (pkt_type == dsp::remote::PKT_TYPE_SOURCESTART)
    {
        source_mtx.lock();
        source_stream_mtx.lock();
        if (!source_is_started)
        {
            current_sample_source->start();
            source_should_stream = true;
            source_is_started = true;
            logger->info("Source started!");
        }
        source_stream_mtx.unlock();
        source_mtx.unlock();
    }

    if (pkt_type == dsp::remote::PKT_TYPE_SOURCESTOP)
    {
        source_mtx.lock();
        if (source_is_started)
        {
            source_should_stream = false;
            source_stream_mtx.lock();
            current_sample_source->stop();
            current_sample_source->output_stream->stopReader();
            current_sample_source->output_stream->stopWriter();
            source_stream_mtx.unlock();
            logger->info("Source stopped!");
            source_is_started = false;
        }
        source_mtx.unlock();
    }

    if (pkt_type == dsp::remote::PKT_TYPE_GUI)
    {
        logger->info("GUI_FEEDBACK");
        gui_feedback_mtx.lock();
        last_draw_feedback = RImGui::decode_vec(buffer + 1, len - 1);
        gui_feedback_mtx.unlock();
    }

    if (pkt_type == dsp::remote::PKT_TYPE_SETFREQ)
    {
        source_mtx.lock();
        double freq = *((double *)&buffer[1]);
        logger->debug("Frequency sent %f", freq);
        if (source_is_open)
            current_sample_source->set_frequency(freq);
        source_mtx.unlock();
    }

    if (pkt_type == dsp::remote::PKT_TYPE_SETSETTINGS)
    {
        source_mtx.lock();
        auto settings = nlohmann::json::from_cbor(std::vector<uint8_t>(&buffer[1], &buffer[len]));
        logger->debug("Setting source settings");
        if (source_is_open)
            current_sample_source->set_settings(settings);
        source_mtx.unlock();
    }

    if (pkt_type == dsp::remote::PKT_TYPE_GETSETTINGS)
    {
        source_mtx.lock();
        nlohmann::json settings;
        logger->debug("Sending source settings");
        if (source_is_open)
            settings = current_sample_source->get_settings();
        sendPacketWithVector(tcp_server, dsp::remote::PKT_TYPE_GETSETTINGS, nlohmann::json::to_cbor(settings));
        source_mtx.unlock();
    }
}

int main(int argc, char *argv[])
{
    initLogger();

    int port_used = std::stoi(argv[1]);

    // We don't wanna spam with init this time around
    logger->set_level(slog::LOG_OFF);
    satdump::initSatdump();
    logger->set_level(slog::LOG_TRACE);

    dsp::registerAllSources();
    dsp::registerAllSinks();

    // Start UDP discovery system
    service_discovery::UDPDiscoveryConfig cfg = {REMOTE_NETWORK_DISCOVERY_PORT, REMOTE_NETWORK_DISCOVERY_REQPKT, REMOTE_NETWORK_DISCOVERY_REPPKT, port_used};
    service_discovery::UDPDiscoveryServerRunner runner(cfg);

    // Start the main TCP Server
    tcp_server = new TCPServer(port_used);
    tcp_server->callback_func = tcp_rx_handler;

    // Start source GUI loop
    std::thread source_gui_th(sourceGuiThread);

    // Start source IQ loop
    std::thread source_iq_th(sourceStreamThread);

    while (1)
        sleep(1);
    return 0;
}
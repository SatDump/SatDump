#include "pkt_handler.h"
#include "logger.h"
#include "main.h"
#include "gui.h"
#include "remote.h"
#include "actions.h"
#include "streaming.h"

std::vector<dsp::SourceDescriptor> sources;

void tcp_rx_handler(uint8_t *buffer, int len)
{
    int pkt_type = buffer[0];

    try
    {
        if (pkt_type == dsp::remote::PKT_TYPE_PING)
        { // Simply reply
            sendPacketWithVector(tcp_server, dsp::remote::PKT_TYPE_PING);
            logger->debug("Ping!");
        }

        if (pkt_type == dsp::remote::PKT_TYPE_SOURCELIST)
        {
            sources = dsp::getAllAvailableSources(true);
            std::vector<dsp::SourceDescriptor> sources_final;

            logger->trace("Found devices (sources) :");
            for (dsp::SourceDescriptor src : sources)
            {
                if (!src.remote_ok)
                    continue;

                logger->trace("- " + src.name);
                sources_final.push_back(src);
            }

            // TODO SIMPLIFY TO BINARY?
            sendPacketWithVector(tcp_server, dsp::remote::PKT_TYPE_SOURCELIST, nlohmann::json::to_cbor(nlohmann::json(sources_final)));
        }

        if (pkt_type == dsp::remote::PKT_TYPE_SOURCEOPEN)
        {
            std::lock_guard<std::mutex> lock(source_mtx);
            if (source_is_open)
            {
                source_is_open = false;
                current_sample_source->close();
                current_sample_source.reset();
                logger->info("Source closed!");
            }

            dsp::SourceDescriptor source = nlohmann::json::from_cbor(std::vector<uint8_t>(&buffer[1], &buffer[len]));

            logger->info("Opening " + source.name);

            if(sources.size() == 0)
                sources = dsp::getAllAvailableSources(true);
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
        }

        if (pkt_type == dsp::remote::PKT_TYPE_SOURCECLOSE)
            action_sourceClose();

        if (pkt_type == dsp::remote::PKT_TYPE_SOURCESTART)
            action_sourceStart();

        if (pkt_type == dsp::remote::PKT_TYPE_SOURCESTOP)
            action_sourceStop();

        if (pkt_type == dsp::remote::PKT_TYPE_GUI)
        {
            // logger->info("GUI_FEEDBACK");
            gui_feedback_mtx.lock();
            last_draw_feedback = RImGui::decode_vec(buffer + 1, len - 1);
            gui_feedback_mtx.unlock();
        }

        if (pkt_type == dsp::remote::PKT_TYPE_SETFREQ)
        {
            std::lock_guard<std::mutex> lock(source_mtx);
            double freq;
            memcpy(&freq, &buffer[1], sizeof(double));
            logger->debug("Frequency sent %f", freq);
            if (source_is_open)
                current_sample_source->set_frequency(freq);
        }

        if (pkt_type == dsp::remote::PKT_TYPE_SETSETTINGS)
        {
            std::lock_guard<std::mutex> lock(source_mtx);
            auto settings = nlohmann::json::from_cbor(std::vector<uint8_t>(&buffer[1], &buffer[len]));
            logger->debug("Setting source settings");
            if (source_is_open)
                current_sample_source->set_settings(settings);
        }

        if (pkt_type == dsp::remote::PKT_TYPE_GETSETTINGS)
        {
            std::lock_guard<std::mutex> lock(source_mtx);
            nlohmann::json settings;
            logger->debug("Sending source settings");
            if (source_is_open)
                settings = current_sample_source->get_settings();
            sendPacketWithVector(tcp_server, dsp::remote::PKT_TYPE_GETSETTINGS, nlohmann::json::to_cbor(settings));
        }

        if (pkt_type == dsp::remote::PKT_TYPE_SAMPLERATESET)
        {
            std::lock_guard<std::mutex> lock(source_mtx);
            memcpy(&last_samplerate, &buffer[1], sizeof(uint64_t));
            logger->debug("Samplerate sent %llu", last_samplerate);
            if (source_is_open)
                current_sample_source->set_samplerate(last_samplerate);

            // Acknowledge the packet to prevent the client from thinking we changed
            // the samplerate if they get a late packet from sourceGuiThread()
            std::vector<uint8_t> pkt(8);
            *((uint64_t*)&pkt[0]) = last_samplerate;
            sendPacketWithVector(tcp_server, dsp::remote::PKT_TYPE_SAMPLERATEFBK, pkt);
        }

        if (pkt_type == dsp::remote::PKT_TYPE_BITDEPTHSET)
        {
            logger->debug("Bit Depth sent %d", buffer[1]);
            streaming_bit_depth = buffer[1];
        }
    }
    catch (std::exception &e)
    {
        logger->error("Error parsing packet from client : %s", e.what());
    }
}
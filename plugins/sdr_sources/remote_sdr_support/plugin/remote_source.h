#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "core/style.h"
#include <fstream>
#include <thread>
#include <atomic>

#include "tcp_test.h"
#include "remote.h"
#include "common/rimgui.h"

#include "iq_pkt.h"

class RemoteSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;

    dsp::SourceDescriptor remote_source_desc;

    TCPClient *tcp_client = nullptr;

    uint64_t samplerate_current = 0;

    std::vector<uint8_t> gui_buffer_tx;
    RImGui::RImGui gui_remote;
    std::mutex drawelems_mtx;
    std::vector<RImGui::UiElem> last_draw_elems;

    std::atomic<bool> waiting_for_settings;

public:
    RemoteSource(dsp::SourceDescriptor source)
        : DSPSampleSource(source)
    {
        std::string ip_port = source.name.substr(0, source.name.find('-') - 1);
        std::string ip = ip_port.substr(0, ip_port.find(':'));
        std::string port = ip_port.substr(ip_port.find(':') + 1, ip_port.size() - 1 - ip_port.find(':'));
        logger->info("Connecting to tcp://" + ip_port);
        tcp_client = new TCPClient((char *)ip.c_str(), std::stoi(port));
        tcp_client->callback_func = [this](uint8_t *buf, int len)
        { handler(buf, len); };

        remote_source_desc = source;
        remote_source_desc.name = source.name.substr(source.name.find('-') + 2, source.name.size() - 2 - source.name.find('-'));
    }

    void handler(uint8_t *buffer, int len)
    {
        int pkt_type = buffer[0];

        if (pkt_type == dsp::remote::PKT_TYPE_PING)
            logger->debug("Pong!");

        if (pkt_type == dsp::remote::PKT_TYPE_GUI)
        {
#if 0
                bool is_zero = false;
                while (!is_zero)
                {
                    drawelems_mtx.lock();
                    is_zero = last_draw_elems.size() == 0;
                    drawelems_mtx.unlock();
                    // sleep(1);
                }
#endif
            drawelems_mtx.lock();
            last_draw_elems = RImGui::decode_vec(buffer + 1, len - 1);
            logger->info("DrawElems %d ----- %d", last_draw_elems.size(), len);
            drawelems_mtx.unlock();
        }

        if (pkt_type == dsp::remote::PKT_TYPE_SAMPLERATEFBK)
        {
            samplerate_current = *((uint64_t *)&buffer[1]);
            logger->debug("Samplerate sent %llu", samplerate_current);
        }

        if (pkt_type == dsp::remote::PKT_TYPE_IQ)
        {
            int nsamples = 0;
            remote_sdr::decode_iq_pkt(&buffer[1], output_stream->writeBuf, &nsamples);
            // memcpy(output_stream->writeBuf, &buffer[3], nsamples * sizeof(complex_t));
            // logger->trace("SAMPLES %d %d", nsamples, len);
            output_stream->swap(nsamples);
        }

        if (pkt_type == dsp::remote::PKT_TYPE_GETSETTINGS)
        {
            logger->debug("Got source settings");
            d_settings = nlohmann::json::from_cbor(std::vector<uint8_t>(&buffer[1], &buffer[len]));
            waiting_for_settings = false;
        }
    }

    ~RemoteSource()
    {
        stop();
        close();

        tcp_client->readOne = true; // Ask to gracefully disconnect
        sendPacketWithVector(tcp_client, dsp::remote::PKT_TYPE_PING);
        if (tcp_client != nullptr)
            delete tcp_client;
    }

    void set_settings(nlohmann::json settings);
    nlohmann::json get_settings();

    void open();
    void start();
    void stop();
    void close();

    void set_frequency(uint64_t frequency);

    void drawControlUI();

    void set_samplerate(uint64_t samplerate);
    uint64_t get_samplerate();

    static std::string getID() { return "remote"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<RemoteSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};
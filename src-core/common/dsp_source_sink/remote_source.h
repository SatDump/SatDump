#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "core/style.h"
#include <fstream>
#include <thread>

#include "common/net/tcp_test.h"
#include "remote.h"

class RemoteSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;

    TCPClient *tcp_client = nullptr;

    uint64_t samplerate_current = 0;

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
    }

    void handler(uint8_t *buffer, int len)
    {
        int pkt_type = buffer[0];

        if (pkt_type == dsp::remote::PKT_TYPE_PING)
            logger->debug("Pong!");
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
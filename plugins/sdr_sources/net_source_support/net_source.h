#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#include "common/widgets/notated_num.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "core/style.h"
#include "common/net/udp.h"
#include <thread>

#include <nng/nng.h>
#include <nng/protocol/pubsub0/sub.h>

class NetSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;

    enum netsource_mode_t
    {
        MODE_UDP,
        MODE_NNGSUB,
    };

    netsource_mode_t mode = MODE_UDP;
    std::string address = "localhost";
    int port = 8877;

    std::shared_ptr<net::UDPServer> udp_server;

    nng_socket n_sock;
    nng_dialer n_dialer;

    widgets::NotatedNum<uint64_t> current_samplerate = widgets::NotatedNum<uint64_t>("Samplerate##net", 0, "sps");

    std::string error;

protected:
    bool should_run = true;
    std::thread work_thread;
    void run_thread();

public:
    NetSource(dsp::SourceDescriptor source) : DSPSampleSource(source)
    {
        should_run = true;
        work_thread = std::thread(&NetSource::run_thread, this);
    }

    ~NetSource()
    {
        stop();
        close();
        should_run = false;
        if (work_thread.joinable())
            work_thread.join();
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

    static std::string getID() { return "net_source"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<NetSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};

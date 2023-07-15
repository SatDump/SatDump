#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "core/style.h"
#include "common/net/udp.h"
#include <thread>

class UDPSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;
    std::shared_ptr<net::UDPServer> udp_server;

    int current_samplerate = 0;

    int port = 8877;

    std::string error;

protected:
    bool should_run = true;
    std::thread work_thread;
    void run_thread();

public:
    UDPSource(dsp::SourceDescriptor source) : DSPSampleSource(source)
    {
        should_run = true;
        work_thread = std::thread(&UDPSource::run_thread, this);
    }

    ~UDPSource()
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

    static std::string getID() { return "udp_source"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<UDPSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};
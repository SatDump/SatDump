#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#include "rtltcp_client.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "core/style.h"
#include <thread>

class RTLTCPSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;
    RTLTCPClient client;

    int selected_samplerate = 0;
    std::string samplerate_option_str;
    std::vector<uint64_t> available_samplerates;
    uint64_t current_samplerate = 0;

    std::string ip_address = "0.0.0.0";
    int port = 1234;
    int gain = 10;
    bool lna_agc_enabled = false;
    bool bias = false;

    void set_gains();
    void set_bias();

    std::thread work_thread;

    bool thread_should_run = false;

    void mainThread()
    {
        uint8_t in_buf[8192];
        while (thread_should_run)
        {
                client.receiveData(in_buf, 8192);
                for (int i = 0; i < (int)8192 / 2; i++)
                    output_stream->writeBuf[i] = complex_t((in_buf[i * 2 + 0] - 127.0f) / 128.0f, (in_buf[i * 2 + 1] - 127.0f) / 128.0f);
                output_stream->swap(8192 / 2);
        }
    }

public:
    RTLTCPSource(dsp::SourceDescriptor source) : DSPSampleSource(source)
    {
    }

    ~RTLTCPSource()
    {
        stop();
        close();
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

    static std::string getID() { return "rtltcp"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<RTLTCPSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};
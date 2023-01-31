#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#ifdef __ANDROID__
#include "mirisdr/mirisdr.h"
#else
#include "mirisdr/mirisdr.h"
#endif
#include "logger.h"
#include "imgui/imgui.h"
#include "core/style.h"
#include <thread>

class MiriSdrSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;
    mirisdr_dev *mirisdr_dev_obj;
    static void _rx_callback_8(unsigned char *buf, uint32_t len, void *ctx);
    static void _rx_callback_16(unsigned char *buf, uint32_t len, void *ctx);

    int selected_samplerate = 0;
    std::string samplerate_option_str;
    std::vector<uint64_t> available_samplerates;
    uint64_t current_samplerate = 0;

    int bit_depth = 12;
    int gain = 0;
    bool bias_enabled = false;

    void set_gains();
    void set_bias();

    std::thread work_thread;

    bool thread_should_run = false;

    bool async_running = false;

    void mainThread()
    {
        int buffer_size = std::min<int>(current_samplerate / 250, dsp::STREAM_BUFFER_SIZE);

        while (thread_should_run)
        {
            logger->trace("Starting async reads...");
            if (bit_depth == 8)
                mirisdr_read_async(mirisdr_dev_obj, _rx_callback_8, &output_stream, 15, buffer_size);
            else
                mirisdr_read_async(mirisdr_dev_obj, _rx_callback_16, &output_stream, 15, buffer_size);
            logger->trace("Stopped async reads...");
        }
    }

public:
    MiriSdrSource(dsp::SourceDescriptor source) : DSPSampleSource(source)
    {
    }

    ~MiriSdrSource()
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

    static std::string getID() { return "mirisdr"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<MiriSdrSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};
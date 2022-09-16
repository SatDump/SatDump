#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#ifdef __ANDROID__
#include "API/lms7_device.h"
#include "protocols/Streamer.h"
#include "ConnectionRegistry/ConnectionRegistry.h"
#else
#include <lime/lms7_device.h>
#include <lime/Streamer.h>
#include <lime/ConnectionRegistry.h>
#endif
#include "logger.h"
#include "imgui/imgui.h"
#include "core/style.h"

class LimeSDRSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;
    lime::LMS7_Device *limeDevice;
    lime::StreamChannel *limeStream;
    lime::StreamChannel *limeStreamID;
    lime::StreamConfig limeConfig;

    int selected_samplerate = 0;
    std::string samplerate_option_str;
    std::vector<uint64_t> available_samplerates;
    uint64_t current_samplerate = 0;

    int tia_gain = 0;
    int lna_gain = 0;
    int pga_gain = 0;

    void set_gains();

    std::thread work_thread;
    bool thread_should_run = false, needs_to_run = false;
    void mainThread()
    {
        lime::StreamChannel::Metadata md;

        while (thread_should_run)
        {
            if (needs_to_run)
            {
                int cnt = limeStream->Read(output_stream->writeBuf, 8192 * 10, &md);
                output_stream->swap(cnt);
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }

public:
    LimeSDRSource(dsp::SourceDescriptor source) : DSPSampleSource(source)
    {
        thread_should_run = true;
        work_thread = std::thread(&LimeSDRSource::mainThread, this);
    }

    ~LimeSDRSource()
    {
        stop();
        close();

        thread_should_run = false;
        logger->info("Waiting for the thread...");
        if (is_started)
            output_stream->stopWriter();
        if (work_thread.joinable())
            work_thread.join();
        logger->info("Thread stopped");
    }

    void set_settings(nlohmann::json settings);
    nlohmann::json get_settings(nlohmann::json);

    void open();
    void start();
    void stop();
    void close();

    void set_frequency(uint64_t frequency);

    void drawControlUI();

    void set_samplerate(uint64_t samplerate);
    uint64_t get_samplerate();

    static std::string getID() { return "limesdr"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<LimeSDRSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};
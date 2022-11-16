#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#ifdef __ANDROID__
#include "API/lms7_device.h"
#include "protocols/Streamer.h"
#include "ConnectionRegistry/ConnectionRegistry.h"
#else
#include <lime/LimeSuite.h>
#endif
#include <thread>
#include "logger.h"
#include "imgui/imgui.h"
#include "core/style.h"

class LimeSDRSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;

    lms_device_t *limeDevice;
    lms_stream_t limeStream;

    int selected_samplerate = 0;
    std::string samplerate_option_str;
    std::vector<uint64_t> available_samplerates;
    uint64_t current_samplerate = 0;

    int gain = 0;

    void set_gains();

    std::thread work_thread;
    bool thread_should_run = false;
    void mainThread()
    {
        lms_stream_meta_t md;

        while (thread_should_run)
        {
            int cnt = LMS_RecvStream(&limeStream, output_stream->writeBuf, 8192 * 10, &md, 2000);
            if (cnt > 0)
                output_stream->swap(cnt);
        }
    }

public:
    LimeSDRSource(dsp::SourceDescriptor source) : DSPSampleSource(source)
    {
        }

    ~LimeSDRSource()
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

    static std::string getID() { return "limesdr"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<LimeSDRSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};
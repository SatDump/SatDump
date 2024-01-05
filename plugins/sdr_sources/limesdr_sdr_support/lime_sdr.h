#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#ifdef __ANDROID__
#include "API/lms7_device.h"
#include "protocols/Streamer.h"
#include "ConnectionRegistry/ConnectionRegistry.h"
#else
#include <lime/LimeSuite.h>
#include <lime/lms7_device.h>
#endif
#include <thread>
#include "logger.h"
#include "common/rimgui.h"
#include "common/widgets/double_list.h"

class LimeSDRSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;

#if __ANDROID__
    lime::LMS7_Device *limeDevice;
    lime::StreamChannel *limeStream;
    lime::StreamChannel *limeStreamID;
    lime::StreamConfig limeConfig;
#else
    lms_device_t *limeDevice;
    lms_stream_t limeStream;
#endif

    widgets::DoubleList samplerate_widget;
    widgets::DoubleList bandwidth_widget;

    int channel_id = 0;

    int path_id = 3;

    bool gain_mode_manual = false;
    int gain_lna = 0, gain_tia = 0, gain_pga = 0;
    int gain = 0;

    bool manual_bandwidth = false;

    void set_gains();
    void set_others();

    std::thread work_thread;
    bool thread_should_run = false;
    void mainThread()
    {
#if __ANDROID__
        lime::StreamChannel::Metadata md;
#else
        lms_stream_meta_t md;
#endif

        int buffer_size = std::min<int>(samplerate_widget.get_value() / 250, dsp::STREAM_BUFFER_SIZE);

        while (thread_should_run)
        {
#if __ANDROID__
            int cnt = limeStream->Read(output_stream->writeBuf, buffer_size, &md);
#else
            int cnt = LMS_RecvStream(&limeStream, output_stream->writeBuf, buffer_size, &md, 2000);
#endif
            if (cnt > 0)
                output_stream->swap(cnt);
        }
    }

public:
    LimeSDRSource(dsp::SourceDescriptor source) : DSPSampleSource(source), samplerate_widget("Samplerate"), bandwidth_widget("Bandwidth")
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
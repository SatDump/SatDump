#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#ifdef __ANDROID__
#include "rtl-sdr.h"
#else
#include <rtl-sdr.h>
#endif
#include "logger.h"
#include "common/rimgui.h"
#include <thread>
#include "common/widgets/double_list.h"

class RtlSdrSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;
    rtlsdr_dev *rtlsdr_dev_obj;
    static void _rx_callback(unsigned char *buf, uint32_t len, void *ctx);

    widgets::DoubleList samplerate_widget;
    widgets::NotatedNum<int> ppm_widget;

    int gain = 0;
    bool bias_enabled = false;
    bool lna_agc_enabled = false;

    void set_gains();
    void set_bias();
    void set_ppm();

    std::thread work_thread;

    bool thread_should_run = false;

    void mainThread()
    {
        int buffer_size = 16384;

        while (thread_should_run)
        {
            rtlsdr_read_async(rtlsdr_dev_obj, _rx_callback, &output_stream, 0, buffer_size);
        }
    }

public:
    RtlSdrSource(dsp::SourceDescriptor source) : DSPSampleSource(source), samplerate_widget("Samplerate"), ppm_widget("Correction##ppm", 0, "ppm")
    {
    }

    ~RtlSdrSource()
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

    static std::string getID() { return "rtlsdr"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<RtlSdrSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};

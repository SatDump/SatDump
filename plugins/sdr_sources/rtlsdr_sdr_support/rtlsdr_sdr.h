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

    int gain[7] = { 0, 0, 0, 0, 0, 0, 0 };
    int last_ppm = 0;
    bool tuner_is_e4000 = false;
    float display_gain = 0.0f;
    float display_gain_e4000[6] = { -30.0f, 0.0f, 0.0f, 0.0f, 30.0f, 30.0f };
    float gain_step = 1.0f;
    std::vector<int> available_gains = { 0, 496 };
    std::vector<std::vector<int>> available_gains_e4000 = {
        {-30, 60 },
        { 0, 30, 60, 90 },
        { 0, 30, 60, 90 },
        { 0, 10, 20 },
        { 30, 60, 90, 120, 150 },
        { 30, 60, 90, 120, 150 }
    };
    bool changed_agc = true;
    bool bias_enabled = false;
    bool lna_agc_enabled = false;
    bool tuner_agc_enabled = false;
    bool changed_offset_tuning = true;
    bool offset_tuning_enabled = false;

    void set_gain(std::vector<int> available_gain, float setgain, bool changed_agc, bool tuner_agc_enabled, bool e4000, int e4000_stage);
    void set_gains();
    void set_bias();
    void set_ppm();

    std::thread work_thread;

    bool thread_should_run = false;

    void mainThread()
    {
        int buffer_size = calculate_buffer_size_from_samplerate(samplerate_widget.get_value());
        // std::min<int>(roundf(samplerate_widget.get_value() / (250 * 512)) * 512, dsp::STREAM_BUFFER_SIZE);
        logger->trace("RTL-SDR Buffer size %d", buffer_size);

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
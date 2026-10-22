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
#include <atomic>
#include "common/widgets/double_list.h"

class RtlSdrSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;
    rtlsdr_dev *rtlsdr_dev_obj = nullptr;
    static void _rx_callback(unsigned char *buf, uint32_t len, void *ctx);

    satdump::widgets::DoubleList samplerate_widget;
    satdump::widgets::NotatedNum<int> ppm_widget;

    int gain = 0;
    int last_ppm = 0;
    float display_gain = 0.0f;
    float gain_step = 1.0f;
    std::vector<int> available_gains = { 0, 496 };
    bool changed_agc = true;
    bool bias_enabled = false;
    bool lna_agc_enabled = false;
    bool tuner_agc_enabled = false;

    void set_gains();
    void set_bias();
    void set_ppm();

    std::thread work_thread;

    std::atomic<bool> thread_should_run{false};
    std::atomic<bool> thread_exited{true};
#ifdef __ANDROID__
    bool permission_requested = false;
#endif

    void mainThread()
    {
        int buffer_size = calculate_buffer_size_from_samplerate(samplerate_widget.get_value());
        // std::min<int>(roundf(samplerate_widget.get_value() / (250 * 512)) * 512, dsp::STREAM_BUFFER_SIZE);
        logger->trace("RTL-SDR Buffer size %d", buffer_size);

        while (thread_should_run)
        {
            int rc = rtlsdr_read_async(rtlsdr_dev_obj, _rx_callback, &output_stream, 0, buffer_size);
            if (rc != 0 || thread_should_run)
            {
                if (thread_should_run)
                    logger->error("RTL-SDR reception ended (USB disconnected or read result %d); stop and reconnect", rc);
                thread_should_run = false;
                output_stream->stopWriter();
                break;
            }
        }
        thread_exited = true;
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
#ifdef __ANDROID__
    bool needs_async_start() override { return true; }
    bool ready_to_start() override;
    void cancel_prepare_start() override { permission_requested = false; }
    bool stream_has_ended() override { return is_started && thread_exited && !thread_should_run; }
#endif
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
#pragma once

#include "common/dsp_source_sink/dsp_sample_sink.h"
#include <libbladeRF.h>
#include "logger.h"
#include "imgui/imgui.h"
#include "core/style.h"
#include <thread>

class BladeRFSink : public dsp::DSPSampleSink
{
protected:
    bool is_open = false, is_started = false;
    bladerf *bladerf_dev_obj;
    int bladerf_model = 0;
    int channel_cnt = 1;
    const bladerf_range *bladerf_range_samplerate;
    const bladerf_range *bladerf_range_bandwidth;
    const bladerf_range *bladerf_range_gain;

    int devs_cnt = 0;
    int selected_dev_id = 0;
    bladerf_devinfo *devs_list = NULL;

    int selected_samplerate = 0;
    std::string samplerate_option_str;
    std::vector<uint64_t> available_samplerates;
    uint64_t current_samplerate = 0;

    int channel_id = 0;
    int gain_mode = 1;
    int general_gain = 0;

    bool bias_enabled = false;

    void set_gains();
    void set_bias();

    bool is_8bit = false;

    std::thread work_thread;
    bool thread_should_run = false;
    void mainThread()
    {
        int16_t *sample_buffer;
        int8_t *sample_buffer_8;
        bladerf_metadata meta;

        if (is_8bit)
        {
            sample_buffer_8 = new int8_t[dsp::STREAM_BUFFER_SIZE * 2];

            while (thread_should_run)
            {
                int nsamples = input_stream->read();
                volk_32f_s32f_convert_8i(sample_buffer_8, (float *)input_stream->readBuf, 127.0f, nsamples * 2);
                if (bladerf_sync_tx(bladerf_dev_obj, sample_buffer_8, nsamples, &meta, 4000) != 0)
                    std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            delete[] sample_buffer_8;
        }
        else
        {
            sample_buffer = new int16_t[dsp::STREAM_BUFFER_SIZE * 2];

            while (thread_should_run)
            {
                int nsamples = input_stream->read();
                volk_32f_s32f_convert_16i(sample_buffer, (float *)input_stream->readBuf, 4096.0f, nsamples * 2);
                if (bladerf_sync_tx(bladerf_dev_obj, sample_buffer, nsamples, &meta, 4000) != 0)
                    std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            delete[] sample_buffer;
        }
    }

public:
    BladeRFSink(dsp::SinkDescriptor sink) : DSPSampleSink(sink)
    {
    }

    ~BladeRFSink()
    {
        stop();
        close();
    }

    void set_settings(nlohmann::json settings);
    nlohmann::json get_settings();

    void open();
    void start(std::shared_ptr<dsp::stream<complex_t>> stream);
    void stop();
    void close();

    void set_frequency(uint64_t frequency);

    void drawControlUI();

    void set_samplerate(uint64_t samplerate);
    uint64_t get_samplerate();

    static std::string getID() { return "bladerf"; }
    static std::shared_ptr<dsp::DSPSampleSink> getInstance(dsp::SinkDescriptor sink) { return std::make_shared<BladeRFSink>(sink); }
    static std::vector<dsp::SinkDescriptor> getAvailableSinks();
};
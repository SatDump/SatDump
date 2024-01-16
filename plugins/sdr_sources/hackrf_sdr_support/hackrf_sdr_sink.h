#pragma once

#include "common/dsp_source_sink/dsp_sample_sink.h"
#ifdef __ANDROID__
#include "hackrf.h"
#else
#include <libhackrf/hackrf.h>
#endif
#include "logger.h"
#include "imgui/imgui.h"
#include "core/style.h"
#include <thread>

class HackRFSink : public dsp::DSPSampleSink
{
protected:
    bool is_open = false, is_started = false;
    hackrf_device *hackrf_dev_obj;
    static int _tx_callback(hackrf_transfer *t);

    int selected_samplerate = 0, selected_bw = 0;
    bool enable_experimental_samplerates = false;
    std::string bandwidth_option_str, samplerate_option_str, samplerate_option_str_exp;
    std::vector<uint64_t> available_bandwidths, available_samplerates, available_samplerates_exp;
    uint64_t current_samplerate = 0;

    int lna_gain = 0;
    int vga_gain = 0;
    uint64_t manual_bw_value = 0;

    bool amp_enabled = false;
    bool bias_enabled = false;
    bool manual_bandwidth = false;

    void set_gains();
    void set_bias();
    void set_others();

    dsp::RingBuffer<uint8_t> fifo_out;
    int8_t *repack_buffer;
    std::thread repack_thread;
    bool should_exit, should_run;
    void repack_th_fun()
    {
        while (!should_exit)
        {
            if (should_run)
            {
                int nsamples = input_stream->read();
                for (int i = 0; i < nsamples; i++)
                {
                    float re = input_stream->readBuf[i].real * 127.0f * 0.5;
                    float im = input_stream->readBuf[i].imag * 127.0f * 0.5;

                    if (re > 127)
                        re = 127;
                    if (im > 127)
                        im = 127;
                    if (re < -127)
                        re = -127;
                    if (im < -127)
                        im = -127;

                    repack_buffer[i * 2 + 0] = re;
                    repack_buffer[i * 2 + 1] = im;
                }
                fifo_out.write((uint8_t *)repack_buffer, nsamples * 2);
                input_stream->flush();
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }

public:
    HackRFSink(dsp::SinkDescriptor sink) : DSPSampleSink(sink)
    {
        fifo_out.init(262144 * 10);
        repack_buffer = new int8_t[8192 * 100];

        should_run = false;
        should_exit = false;
        repack_thread = std::thread(&HackRFSink::repack_th_fun, this);
    }

    ~HackRFSink()
    {
        should_exit = true;
        if (repack_thread.joinable())
            repack_thread.join();
        stop();
        close();
        delete[] repack_buffer;
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

    static std::string getID() { return "hackrf"; }
    static std::shared_ptr<dsp::DSPSampleSink> getInstance(dsp::SinkDescriptor source) { return std::make_shared<HackRFSink>(source); }
    static std::vector<dsp::SinkDescriptor> getAvailableSinks();
};
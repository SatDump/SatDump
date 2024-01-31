#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#include "logger.h"
#include "common/rimgui.h"
#include <thread>
#include "common/widgets/double_list.h"
#include <SoapySDR/Device.hpp>
#include <SoapySDR/Modules.hpp>
#include <SoapySDR/Logger.hpp>

class SoapySdrSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;
    std::string d_source_label;

    SoapySDR::Device *soapy_dev = nullptr;
    SoapySDR::Stream *soapy_dev_stream;

    int channel_id = 0;

    std::vector<std::string> antenna_list;
    std::string txt_antenna_list;
    int antenna_id = 0;

    std::vector<std::string> gain_list;
    std::vector<SoapySDR::Range> gain_list_ranges;
    std::vector<float> gain_list_vals;

    bool manual_bandwidth = false;
    widgets::DoubleList bandwidth_widget;
    widgets::DoubleList samplerate_widget;

    std::vector<std::string> list_options_bool;
    std::vector<std::string> list_options_bool_ids;
    bool list_options_bool_vals[100];

    std::vector<std::string> list_options_strings;
    std::vector<std::string> list_options_strings_ids;
    std::vector<std::vector<std::string>> list_options_strings_options;
    std::vector<std::string> list_options_strings_options_txt;
    std::vector<int> list_options_strings_values;

    void set_params();

    std::thread work_thread;

    bool thread_should_run = false;

    void mainThread()
    {
        int buffer_size = calculate_buffer_size_from_samplerate(samplerate_widget.get_value());
        // int buffer_size = std::min<int>(roundf(samplerate_widget.get_value() / (250 * 512)) * 512, dsp::STREAM_BUFFER_SIZE);
        logger->trace("SoapySDR Buffer size %d", buffer_size);

        int flags = 0;
        long long time_ms = 0;

        while (thread_should_run)
        {
            int samples = soapy_dev->readStream(soapy_dev_stream, (void **)&output_stream->writeBuf, buffer_size, flags, time_ms);
            if (samples > 0)
                output_stream->swap(samples);
        }
    }

    SoapySDR::Device *get_device(std::string label)
    {
        SoapySDR::Device *ptr = nullptr;
        auto devices = SoapySDR::Device::enumerate();
        for (auto &dev : devices)
        {
            std::string name = (dev["label"] != "" ? dev["label"] : dev["driver"]) + " [Soapy]";
            if (name == label)
            {
                ptr = SoapySDR::Device::make(dev);
                continue;
            }
        }
        return ptr;
    }

public:
    SoapySdrSource(dsp::SourceDescriptor source) : DSPSampleSource(source), bandwidth_widget("Bandwidth"), samplerate_widget("Samplerate")
    {
        d_source_label = source.name;
    }

    ~SoapySdrSource()
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

    static std::string getID() { return "soapysdr"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<SoapySdrSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};
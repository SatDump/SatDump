#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#include <uhd.h>
#include <uhd/device.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include "logger.h"
#include "imgui/imgui.h"
#include "core/style.h"
#include <thread>

class USRPSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;

    uhd::usrp::multi_usrp::sptr usrp_device;
    uhd::rx_streamer::sptr usrp_streamer;
    uhd::gain_range_t gain_range;

    bool use_device_rates = false;

    int selected_samplerate = 0;
    std::string samplerate_option_str;
    std::vector<uint64_t> available_samplerates;
    uint64_t current_samplerate = 0;

    std::string channel_option_str;
    std::vector<std::string> usrp_antennas;
    std::string antenna_option_str;
    int selected_bit_depth = 1;

    int channel = 0;
    int antenna = 0;
    float gain = 0;
    int bit_depth = 16;

    void set_gains();

    void open_sdr();
    void open_channel();

    std::thread work_thread;
    bool thread_should_run = false;
    void mainThread()
    {
        uhd::rx_metadata_t meta;

        int buffer_size = std::min<int>(current_samplerate / 250, STREAM_BUFFER_SIZE);

        while (thread_should_run)
        {
            int cnt = usrp_streamer->recv(output_stream->writeBuf, buffer_size, meta, 1.0);
            if (cnt > 0)
                output_stream->swap(cnt);
            if (meta.error_code != meta.ERROR_CODE_NONE)
                logger->error("USRP Stream error {:s}", meta.strerror().c_str());
        }
    }

public:
    USRPSource(dsp::SourceDescriptor source) : DSPSampleSource(source) {}

    ~USRPSource()
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

    static std::string getID() { return "usrp"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<USRPSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};
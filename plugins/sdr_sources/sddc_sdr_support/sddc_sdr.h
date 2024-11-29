#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#include "lib/libsddc/libsddc.h"
#include "logger.h"
#include "common/rimgui.h"
#include "common/dsp/resamp/rational_resampler.h"

class SDDCSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;
    sddc_t *sddc_dev_obj;
    static void _rx_callback(uint32_t data_size, const float *data, void *context);
    static complex_t ddc_phase_delta;
    static complex_t ddc_phase;

    int selected_samplerate = 0;
    std::string samplerate_option_str;
    std::vector<uint64_t> available_samplerates;
    uint64_t current_samplerate = 0;

    std::shared_ptr<dsp::stream<complex_t>> sddc_stream;
    std::shared_ptr<dsp::RationalResamplerBlock<complex_t>> resampler; // We need to decimate by 2!

    int mode = 0;
    int rf_gain = 0;
    int if_gain = 0;
    bool bias = false;

    void set_att();
    void set_bias();

public:
    SDDCSource(dsp::SourceDescriptor source) : DSPSampleSource(source)
    {
        sddc_stream = std::make_shared<dsp::stream<complex_t>>();
        resampler = std::make_shared<dsp::RationalResamplerBlock<complex_t>>(sddc_stream, 1, 2);
    }

    ~SDDCSource()
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

    static std::string getID() { return "sddc"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<SDDCSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};
#pragma once

#include "common/dsp_sample_source/dsp_sample_source.h"
#include <libairspy/airspy.h>
#include "logger.h"
#include "imgui/imgui.h"
#include "core/style.h"

class AirspySource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;
    airspy_device *airspy_dev_obj;
    static int _rx_callback(airspy_transfer *t);

    int selected_samplerate = 0;
    std::string samplerate_option_str;
    std::vector<uint64_t> available_samplerates;
    uint64_t current_samplerate = 0;

    int gain_type = 0;
    int general_gain = 0;
    int manual_gains[3] = {0, 0, 0};

    bool bias_enabled = false;
    bool lna_agc_enabled = false;
    bool mixer_agc_enabled = false;

    void set_gains();
    void set_bias();
    void set_agcs();

public:
    AirspySource(dsp::SourceDescriptor source) : DSPSampleSource(source) {}

    ~AirspySource()
    {
        stop();
        close();
    }

    void set_settings(nlohmann::json settings);
    nlohmann::json get_settings(nlohmann::json);

    void open();
    void start();
    void stop();
    void close();

    void set_frequency(uint64_t frequency);

    void drawControlUI();

    void set_samplerate(uint64_t samplerate);
    uint64_t get_samplerate();

    static std::string getID() { return "airspy"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<AirspySource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};
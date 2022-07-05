#pragma once

#include "common/dsp_sample_source/dsp_sample_source.h"
#ifdef __ANDROID__
#include "hackrf.h"
#else
#include <libhackrf/hackrf.h>
#endif
#include "logger.h"
#include "imgui/imgui.h"
#include "core/style.h"

class HackRFSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;
    hackrf_device *hackrf_dev_obj;
    static int _rx_callback(hackrf_transfer *t);

    int selected_samplerate = 0;
    bool enable_experimental_samplerates = false;
    std::string samplerate_option_str, samplerate_option_str_exp;
    std::vector<uint64_t> available_samplerates, available_samplerates_exp;
    uint64_t current_samplerate = 0;

    int lna_gain = 0;
    int vga_gain = 0;

    bool amp_enabled = false;
    bool bias_enabled = false;

    void set_gains();
    void set_bias();

public:
    HackRFSource(dsp::SourceDescriptor source) : DSPSampleSource(source) {}

    ~HackRFSource()
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

    static std::string getID() { return "hackrf"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<HackRFSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};
/*
* Plugin for Harogic SA devices
* Developped by Sebastien Dudek (@FlUxIuS) at @Penthertz
*/
#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#include "logger.h"
#include "common/rimgui.h"
#include <thread>
#include <atomic>
#include <vector>
#include <cinttypes> // For PRIX64

// Include the Harogic HTRA API
#include <htra_api.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#define RESOLTRIG 62e6 // Threshold for 8-bit resolution

class HarogicSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;
    void* dev_handle = nullptr;
    DeviceInfo_TypeDef dev_info;
    IQS_Profile_TypeDef profile;
    int dev_index = -1;
    bool samps_int8 = false;

    // UI and settings variables
    int selected_samplerate_idx = 0;
    std::string samplerate_option_str;
    std::vector<uint64_t> available_samplerates;
    uint64_t current_samplerate = 0;

    // Harogic-specific settings
    float d_ref_level = -20;
    int d_preamp = 0; // 0=Off, 1=Auto
    int d_if_agc = 0; // 0=Off, 1=On
    int d_gain_strategy = 0; // 0=LowNoise, 1=HighLinearity
    int d_antenna_idx = 0;

    void update_settings();

    // Worker thread for receiving samples
    std::thread work_thread;
    std::atomic<bool> thread_should_run = false;
    void mainThread();

public:
    HarogicSource(dsp::SourceDescriptor source);
    ~HarogicSource();

    void set_settings(nlohmann::json settings) override;
    nlohmann::json get_settings() override;

    void open() override;
    void start() override;
    void stop() override;
    void close() override;

    void set_frequency(uint64_t frequency) override;
    void drawControlUI() override;
    void set_samplerate(uint64_t samplerate) override;
    uint64_t get_samplerate() override;

    static std::string getID() { return "harogic"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<HarogicSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};

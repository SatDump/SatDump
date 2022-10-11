#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#include <sdrplay_api.h>
#include "logger.h"
#include "imgui/imgui.h"
#include "core/style.h"

class SDRPlaySource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;
    sdrplay_api_DeviceT sdrplay_dev;
    sdrplay_api_DeviceParamsT *dev_params = nullptr;
    sdrplay_api_RxChannelParamsT *channel_params = nullptr;
    sdrplay_api_CallbackFnsT callback_funcs;
    static sdrplay_api_DeviceT devices_addresses[128];
    static void event_callback(sdrplay_api_EventT id, sdrplay_api_TunerSelectT tuner, sdrplay_api_EventParamsT *params, void *ctx);
    static void stream_callback(short *real, short *imag, sdrplay_api_StreamCbParamsT *params, unsigned int cnt, unsigned int reset, void *ctx);

    int max_gain;

    int selected_samplerate = 0;
    std::string samplerate_option_str;
    std::vector<uint64_t> available_samplerates;
    uint64_t current_samplerate = 0;

    int lna_gain = 0;
    int if_gain = 20;
    bool bias = false;
    bool fm_notch = false;
    bool dab_notch = false;
    bool am_notch = false;
    int antenna_input = 0;
    int agc_mode = 0;

    void set_gains();
    void set_bias();
    void set_agcs();
    void set_others();

    void set_duo_tuner();
    void set_duo_channel();

public:
    SDRPlaySource(dsp::SourceDescriptor source) : DSPSampleSource(source) {}

    ~SDRPlaySource()
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

    static std::string getID() { return "sdrplay"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<SDRPlaySource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};
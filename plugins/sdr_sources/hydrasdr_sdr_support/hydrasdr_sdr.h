#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#ifdef __ANDROID__
#include "hydrasdr.h"
#else
#include <libhydrasdr/hydrasdr.h>
#endif
#include "logger.h"
#include "common/rimgui.h"
#include "common/widgets/double_list.h"

class HydraSDRSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;
    hydrasdr_device *hydrasdr_dev_obj;
    static int _rx_callback(hydrasdr_transfer *t);

    widgets::DoubleList samplerate_widget;

    int gain_type = 0;
    int general_gain = 0;
    int manual_gains[3] = {0, 0, 0};

    bool bias_enabled = false;
    bool lna_agc_enabled = false;
    bool mixer_agc_enabled = false;

    void set_gains();
    void set_bias();
    void set_agcs();

    void open_sdr();

public:
    HydraSDRSource(dsp::SourceDescriptor source) : DSPSampleSource(source), samplerate_widget("Samplerate")
    {
    }

    ~HydraSDRSource()
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

    static std::string getID() { return "hydrasdr"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<HydraSDRSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};
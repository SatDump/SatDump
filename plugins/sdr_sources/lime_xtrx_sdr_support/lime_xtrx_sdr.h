#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#include <limesuiteng/limesuiteng.hpp>
#include <thread>
#include "logger.h"
#include "common/rimgui.h"
#include "common/widgets/double_list.h"

using namespace lime;

class LimeXTRXSDRSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;

    SDRDevice* limeDevice = nullptr;
    std::unique_ptr<RFStream> limeStream;
    uint8_t moduleIndex = 0;

    widgets::DoubleList samplerate_widget;
    widgets::DoubleList bandwidth_widget;

    int channel_id = 0;

    int path_id = 3;

    bool gain_mode_manual = false;
    int gain_lna = 0, gain_tia = 0, gain_pga = 0;
    int gain = 0;

    bool manual_bandwidth = false;

    void set_gains();
    void set_others();

    std::thread work_thread;
    bool thread_should_run = false;
    void mainThread();

public:
    LimeXTRXSDRSource(dsp::SourceDescriptor source);
    ~LimeXTRXSDRSource();

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

    static std::string getID() { return "limesdr"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<LimeXTRXSDRSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};
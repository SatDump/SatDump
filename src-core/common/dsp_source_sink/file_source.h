#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#include "imgui/imgui.h"
#include <fstream>
#include "common/dsp/io/baseband_interface.h"
#include <thread>
#include "imgui/dialogs/widget.h"
#include "common/widgets/notated_num.h"
#include <chrono>

class FileSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;

    std::chrono::steady_clock::time_point start_time_point;
    std::chrono::duration<double> sample_time_period;
    int buffer_size = 8192;
    std::string file_path;
    bool iq_swap = false, fast_playback = false;

    unsigned long long total_samples = 0;

    FileSelectWidget file_input = FileSelectWidget("Select", "Select Input Baseband");
    widgets::NotatedNum<int> samplerate_input = widgets::NotatedNum("Samplerate", 0, "sps");
    dsp::BasebandType baseband_type = "cf32";

    bool should_run = true;
    std::thread work_thread;
    void run_thread();

    float file_progress = 0;

    dsp::BasebandReader baseband_reader;

    bool is_ui = false;

public:
    FileSource(dsp::SourceDescriptor source);
    ~FileSource();

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

    static std::string getID() { return "file"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<FileSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};
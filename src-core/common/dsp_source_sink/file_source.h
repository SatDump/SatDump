#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "core/style.h"
#include <fstream>
#include "common/dsp/io/baseband_interface.h"
#include <thread>
#include "imgui/pfd/widget.h"

class FileSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;

    int current_samplerate = 0;

    int ns_to_wait;
    int buffer_size = 8192;
    std::string file_path;
    bool iq_swap = false;

    FileSelectWidget file_input = FileSelectWidget("Select", "Select Input Baseband");

    int select_sample_format;
    std::string baseband_type = "f32";
    dsp::BasebandType baseband_type_e;

    bool should_run = true;
    std::thread work_thread;
    void run_thread();

    float file_progress = 0;

    dsp::BasebandReader baseband_reader;

    bool is_ui = false;

public:
    FileSource(dsp::SourceDescriptor source) : DSPSampleSource(source)
    {
        should_run = true;
        work_thread = std::thread(&FileSource::run_thread, this);
    }

    ~FileSource()
    {
        stop();
        close();
        should_run = false;
        if (work_thread.joinable())
            work_thread.join();
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

    static std::string getID() { return "file"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<FileSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};
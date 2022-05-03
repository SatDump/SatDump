#pragma once

#include "common/dsp_sample_source/dsp_sample_source.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "core/style.h"
#include <fstream>
#include "common/dsp/file_source.h"

class FileSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;

    int current_samplerate = 0;

    int ns_to_wait;
    int buffer_size = 8192;
    std::string file_path;

    std::ifstream input_file;

    std::mutex file_mutex;
    float file_progress = 0;
    uint64_t filesize;

    // Int16 buffer
    int16_t *buffer_i16;
    // Int8 buffer
    int8_t *buffer_i8;
    // Uint8 buffer
    uint8_t *buffer_u8;

    int select_sample_format;
    std::string baseband_type = "f32";
    dsp::BasebandType baseband_type_e;

    bool should_run = true;
    std::thread work_thread;
    void run_thread();

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
    nlohmann::json get_settings(nlohmann::json);

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
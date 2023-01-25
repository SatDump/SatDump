#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#include <rtaudio/RtAudio.h>
#include "logger.h"
#include "imgui/imgui.h"
#include "core/style.h"

class RtAudioSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;
    RtAudio adc_dev;
    RtAudio::StreamParameters adc_prm;
    static int callback_stereo(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData);
    static int callback_mono(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData);

    int channel_count = 1;
    int selected_samplerate = 0;
    std::string samplerate_option_str;
    std::vector<uint64_t> available_samplerates;
    uint64_t current_samplerate = 0;

    int channel_mode = 1;

public:
    RtAudioSource(dsp::SourceDescriptor source) : DSPSampleSource(source)
    {
    }

    ~RtAudioSource()
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

    static std::string getID() { return "rtaudio"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<RtAudioSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};
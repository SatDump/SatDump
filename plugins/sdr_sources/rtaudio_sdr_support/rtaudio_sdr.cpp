#include "rtaudio_sdr.h"

int RtAudioSource::callback_stereo(void */*outputBuffer*/, void *inputBuffer, unsigned int nBufferFrames, double /*streamTime*/, RtAudioStreamStatus /*status*/, void *userData)
{
    std::shared_ptr<dsp::stream<complex_t>> stream = *((std::shared_ptr<dsp::stream<complex_t>> *)userData);
    int16_t *buffer = (int16_t *)inputBuffer;
    for (int i = 0; i < (int)nBufferFrames; i++)
        stream->writeBuf[i] = complex_t(buffer[i * 2 + 0] / 32768.0f, buffer[i * 2 + 1] / 32768.0f);
    stream->swap(nBufferFrames);
    return 0;
}

int RtAudioSource::callback_mono(void */*outputBuffer*/, void *inputBuffer, unsigned int nBufferFrames, double /*streamTime*/, RtAudioStreamStatus /*status*/, void *userData)
{
    std::shared_ptr<dsp::stream<complex_t>> stream = *((std::shared_ptr<dsp::stream<complex_t>> *)userData);
    int16_t *buffer = (int16_t *)inputBuffer;
    for (int i = 0; i < (int)nBufferFrames; i++)
        stream->writeBuf[i] = complex_t(buffer[i], 0);
    stream->swap(nBufferFrames);
    return 0;
}

void RtAudioSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    channel_mode = getValueOrDefault(d_settings["channel_cnt"], channel_mode);

    if (is_started)
    {
    }
}

nlohmann::json RtAudioSource::get_settings()
{
    d_settings["channel_cnt"] = channel_mode;

    return d_settings;
}

void RtAudioSource::open()
{
    is_open = true;

    // Set available samplerate
    available_samplerates.clear();

    RtAudio::DeviceInfo info = adc_dev.getDeviceInfo(d_sdr_id);
    for (auto s : info.sampleRates)
        available_samplerates.push_back(s);
    channel_count = info.inputChannels;
    if (channel_count == 2)
        channel_mode = 2;

    // Init UI stuff
    samplerate_option_str = "";
    for (uint64_t samplerate : available_samplerates)
        samplerate_option_str += formatSamplerateToString(samplerate) + '\0';
}

void RtAudioSource::start()
{
    DSPSampleSource::start();

    if (adc_dev.getDeviceCount() < 1)
        throw std::runtime_error("No audio devices found!");

    adc_prm.deviceId = d_sdr_id;
    adc_prm.nChannels = channel_mode;
    adc_prm.firstChannel = 0;

    unsigned int sampleRate = current_samplerate;
    unsigned int bufferFrames = 256; // 256 sample frames

    if (channel_mode == 1)
        adc_dev.openStream(NULL, &adc_prm, RTAUDIO_SINT16, sampleRate, &bufferFrames, &RtAudioSource::callback_mono, &output_stream);
    else if (channel_mode == 2)
        adc_dev.openStream(NULL, &adc_prm, RTAUDIO_SINT16, sampleRate, &bufferFrames, &RtAudioSource::callback_stereo, &output_stream);
    adc_dev.startStream();

    is_started = true;
}

void RtAudioSource::stop()
{
    if (is_started)
    {
        adc_dev.stopStream();
        adc_dev.closeStream();
    }
    is_started = false;
}

void RtAudioSource::close()
{
    is_open = false;
}

void RtAudioSource::set_frequency(uint64_t frequency)
{
    if (is_started)
    {
        // No Freq for audio!
        logger->debug("Set RtAudio frequency to {:d}", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void RtAudioSource::drawControlUI()
{

    if (is_started)
        style::beginDisabled();

    ImGui::Combo("Samplerate", &selected_samplerate, samplerate_option_str.c_str());
    current_samplerate = available_samplerates[selected_samplerate];

    if (channel_count >= 2)
    {
        if (ImGui::RadioButton("Mono##rtaudiomono", channel_mode == 1))
            channel_mode = 1;
        if (ImGui::RadioButton("Stereo##rtaudiomono", channel_mode == 2))
            channel_mode = 2;
    }

    if (is_started)
        style::endDisabled();
}

void RtAudioSource::set_samplerate(uint64_t samplerate)
{
    for (int i = 0; i < (int)available_samplerates.size(); i++)
    {
        if (samplerate == available_samplerates[i])
        {
            selected_samplerate = i;
            current_samplerate = samplerate;
            return;
        }
    }

    throw std::runtime_error("Unspported samplerate : " + std::to_string(samplerate) + "!");
}

uint64_t RtAudioSource::get_samplerate()
{
    return current_samplerate;
}

std::vector<dsp::SourceDescriptor> RtAudioSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    RtAudio adc_dev;
    unsigned int devices = adc_dev.getDeviceCount();
    RtAudio::DeviceInfo info;
    for (unsigned int i = 0; i < devices; i++)
    {
        info = adc_dev.getDeviceInfo(i);
        if (info.probed == true)
            results.push_back({"rtaudio", "RtAudio - " + info.name, i});
    }

    return results;
}
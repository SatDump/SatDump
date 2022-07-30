#include "rtlsdr_sdr.h"

void RtlSdrSource::_rx_callback(unsigned char *buf, uint32_t len, void *ctx)
{
    std::shared_ptr<dsp::stream<complex_t>> stream = *((std::shared_ptr<dsp::stream<complex_t>> *)ctx);
    for (int i = 0; i < (int)len / 2; i++)
        stream->writeBuf[i] = complex_t((buf[i * 2 + 0] - 127) / 128.0f, (buf[i * 2 + 1] - 127) / 128.0f);
    stream->swap(len / 2);
};

void RtlSdrSource::set_gains()
{
    rtlsdr_set_tuner_gain(rtlsdr_dev_obj, gain);
    logger->debug("Set RTL-SDR Gain to {:d}", gain);
}

void RtlSdrSource::set_bias()
{
    rtlsdr_set_bias_tee(rtlsdr_dev_obj, bias_enabled);
    logger->debug("Set RTL-SDR Bias to {:d}", (int)bias_enabled);
}

void RtlSdrSource::set_agcs()
{
    rtlsdr_set_tuner_gain_mode(rtlsdr_dev_obj, !lna_agc_enabled);
    logger->debug("Set RTL-SDR AGC to {:d}", (int)!lna_agc_enabled);
}

void RtlSdrSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    gain = getValueOrDefault(d_settings["gain"], gain);
    lna_agc_enabled = getValueOrDefault(d_settings["agc"], lna_agc_enabled);
    bias_enabled = getValueOrDefault(d_settings["bias"], bias_enabled);

    if (is_open)
    {
        set_gains();
        set_bias();
        set_agcs();
    }
}

nlohmann::json RtlSdrSource::get_settings(nlohmann::json)
{
    d_settings["gain"] = gain;
    d_settings["agc"] = lna_agc_enabled;
    d_settings["bias"] = bias_enabled;

    return d_settings;
}

void RtlSdrSource::open()
{
    if (!is_open)
        if (rtlsdr_open(&rtlsdr_dev_obj, d_sdr_id) != 0)
            throw std::runtime_error("Could not open RTL-SDR device!");
    is_open = true;

    // Set available samplerate
    available_samplerates.clear();
    available_samplerates.push_back(250000);
    available_samplerates.push_back(1024000);
    available_samplerates.push_back(1536000);
    available_samplerates.push_back(1792000);
    available_samplerates.push_back(1920000);
    available_samplerates.push_back(2048000);
    available_samplerates.push_back(2160000);
    available_samplerates.push_back(2400000);
    available_samplerates.push_back(2560000);
    available_samplerates.push_back(2880000);
    available_samplerates.push_back(3200000);

    // Init UI stuff
    samplerate_option_str = "";
    for (uint64_t samplerate : available_samplerates)
        samplerate_option_str += std::to_string(samplerate) + '\0';
}

void RtlSdrSource::start()
{
    DSPSampleSource::start();

    logger->debug("Set RTL-SDR samplerate to " + std::to_string(current_samplerate));
    rtlsdr_set_sample_rate(rtlsdr_dev_obj, current_samplerate);

    set_frequency(d_frequency);

    set_gains();
    set_bias();
    set_agcs();

    rtlsdr_reset_buffer(rtlsdr_dev_obj);
    needs_to_run = true;

    is_started = true;
}

void RtlSdrSource::stop()
{
    needs_to_run = false;
    rtlsdr_cancel_async(rtlsdr_dev_obj);
    is_started = false;
}

void RtlSdrSource::close()
{
    if (is_open)
        rtlsdr_close(rtlsdr_dev_obj);
    is_open = false;
}

void RtlSdrSource::set_frequency(uint64_t frequency)
{
    if (is_open)
    {
        rtlsdr_set_center_freq(rtlsdr_dev_obj, frequency);
        logger->debug("Set RTL-SDR frequency to {:d}", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void RtlSdrSource::drawControlUI()
{
    if (is_started)
        style::beginDisabled();
    ImGui::Combo("Samplerate", &selected_samplerate, samplerate_option_str.c_str());
    current_samplerate = available_samplerates[selected_samplerate];
    if (is_started)
        style::endDisabled();

    if (ImGui::SliderInt("LNA Gain", &gain, 0, 49))
        set_gains();

    if (ImGui::Checkbox("AGC", &lna_agc_enabled))
        set_agcs();

    if (ImGui::Checkbox("Bias-Tee", &bias_enabled))
        set_bias();
}

void RtlSdrSource::set_samplerate(uint64_t samplerate)
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

uint64_t RtlSdrSource::get_samplerate()
{
    return current_samplerate;
}

std::vector<dsp::SourceDescriptor> RtlSdrSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    int c = rtlsdr_get_device_count();

    for (int i = 0; i < c; i++)
    {
        const char *name = rtlsdr_get_device_name(i);
        results.push_back({"rtlsdr", std::string(name) + " #" + std::to_string(i), uint64_t(i)});
    }

    return results;
}
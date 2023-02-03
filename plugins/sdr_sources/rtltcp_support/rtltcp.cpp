#include "rtltcp.h"
#include "imgui/imgui_stdlib.h"

void RTLTCPSource::set_gains()
{
    if (!is_started)
        return;

    client.setAGCMode(lna_agc_enabled);
    logger->debug("Set RTL-TCP AGC to {:d}", (int)lna_agc_enabled);

    client.setGain(gain * 10);
    logger->debug("Set RTL-TCP Gain to {:d}", gain * 10);
}

void RTLTCPSource::set_bias()
{
    if (!is_started)
        return;
    client.setBiasTee(bias);
    logger->debug("Set RTL-TCP Bias to {:d}", (int)bias);
}

void RTLTCPSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    ip_address = getValueOrDefault(d_settings["ip_address"], ip_address);
    port = getValueOrDefault(d_settings["port"], port);
    gain = getValueOrDefault(d_settings["gain"], gain);
    lna_agc_enabled = getValueOrDefault(d_settings["lna_agc"], lna_agc_enabled);
    bias = getValueOrDefault(d_settings["bias"], bias);

    if (is_open && is_started)
    {
        set_gains();
    }
}

nlohmann::json RTLTCPSource::get_settings()
{
    d_settings["ip_address"] = ip_address;
    d_settings["port"] = port;
    d_settings["gain"] = gain;
    d_settings["lna_agc"] = lna_agc_enabled;
    d_settings["bias"] = lna_agc_enabled;

    return d_settings;
}

void RTLTCPSource::open()
{
    // Nothing to do
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
        samplerate_option_str += formatSamplerateToString(samplerate) + '\0';
}

void RTLTCPSource::start()
{
    if (client.connectToRTL((char *)ip_address.c_str(), port) != 1)
        throw std::runtime_error("Could not connect to RTL-TCP");

    DSPSampleSource::start(); // Do NOT reset the stream

    client.setSampleRate(current_samplerate);

    is_started = true;

    set_frequency(d_frequency);

    set_gains();
    set_bias();

    thread_should_run = true;
    work_thread = std::thread(&RTLTCPSource::mainThread, this);
}

void RTLTCPSource::stop()
{
    if (is_started)
    {
        thread_should_run = false;
        logger->info("Waiting for the thread...");
        if (is_started)
            output_stream->stopWriter();
        if (work_thread.joinable())
            work_thread.join();
        logger->info("Thread stopped");

        client.disconnect();
    }
    is_started = false;
}

void RTLTCPSource::close()
{
    is_open = false;
}

void RTLTCPSource::set_frequency(uint64_t frequency)
{
    if (is_open && is_started)
    {
        client.setFrequency(frequency);
        logger->debug("Set RTL-TCP frequency to {:d}", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void RTLTCPSource::drawControlUI()
{
    if (is_started)
        style::beginDisabled();
    ImGui::Combo("Samplerate", &selected_samplerate, samplerate_option_str.c_str());
    current_samplerate = available_samplerates[selected_samplerate];
    if (is_started)
        style::endDisabled();

    if (is_started)
        style::beginDisabled();
    ImGui::InputText("Address", &ip_address);
    ImGui::InputInt("Port", &port);
    if (is_started)
        style::endDisabled();

    if (!is_started)
        style::beginDisabled();
    bool gain_changed = false;
    gain_changed |= ImGui::SliderInt("Gain", &gain, 0, 49);
    gain_changed |= ImGui::Checkbox("AGC", &lna_agc_enabled);
    if (gain_changed)
        set_gains();
    if (!is_started)
        style::endDisabled();
}

void RTLTCPSource::set_samplerate(uint64_t samplerate)
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

uint64_t RTLTCPSource::get_samplerate()
{
    return current_samplerate;
}

std::vector<dsp::SourceDescriptor> RTLTCPSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    results.push_back({"rtltcp", "RTL-TCP", 0});

    return results;
}
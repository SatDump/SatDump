#include "spyserver.h"
#include "imgui/imgui_stdlib.h"

SpyServerStreamFormat depth_to_format(int depth)
{
    if (depth == 4)
        return SpyServerStreamFormat::SPYSERVER_STREAM_FORMAT_DINT4;
    else if (depth == 8)
        return SpyServerStreamFormat::SPYSERVER_STREAM_FORMAT_UINT8;
    else if (depth == 16)
        return SpyServerStreamFormat::SPYSERVER_STREAM_FORMAT_INT16;
    else if (depth == 24)
        return SpyServerStreamFormat::SPYSERVER_STREAM_FORMAT_INT24;
    else if (depth == 32)
        return SpyServerStreamFormat::SPYSERVER_STREAM_FORMAT_FLOAT;
    else
        return SpyServerStreamFormat::SPYSERVER_STREAM_FORMAT_INVALID;
}

void SpyServerSource::set_gains()
{
    client->setSetting(SPYSERVER_SETTING_GAIN, gain);
    if (digital_gain == 0)
        digital_gain = client->computeDigitalGain(depth_to_format(bit_depth), gain, stage_to_use);
    client->setSetting(SPYSERVER_SETTING_IQ_DIGITAL_GAIN, digital_gain);
    logger->debug("Set SpyServer gain (device) to {:d}", gain);
    logger->debug("Set SpyServer gain (digital) to {:d}", digital_gain);
}

void SpyServerSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    ip_address = getValueOrDefault(d_settings["ip_address"], ip_address);
    port = getValueOrDefault(d_settings["port"], port);
    bit_depth = getValueOrDefault(d_settings["bit_depth"], bit_depth);
    gain = getValueOrDefault(d_settings["gain"], gain);
    digital_gain = getValueOrDefault(d_settings["digital_gain"], digital_gain);

    if (is_open && is_connected)
    {
        set_gains();
    }
}

nlohmann::json SpyServerSource::get_settings()
{

    d_settings["ip_address"] = ip_address;
    d_settings["port"] = port;
    d_settings["bit_depth"] = bit_depth;
    d_settings["gain"] = gain;
    d_settings["digital_gain"] = digital_gain;

    return d_settings;
}

void SpyServerSource::open()
{
    // Nothing to do
    is_open = true;
}

void SpyServerSource::start()
{
    if (is_connected)
        disconnect();
    if (!is_connected)
        try_connect();

    // DSPSampleSource::start(); // Do NOT reset the stream

    client->setSetting(SPYSERVER_SETTING_IQ_FORMAT, depth_to_format(bit_depth));
    client->setSetting(SPYSERVER_SETTING_STREAMING_MODE, SPYSERVER_STREAM_MODE_IQ_ONLY);

    logger->debug("Set SpyServer samplerate to " + std::to_string(current_samplerate));
    client->setSetting(SPYSERVER_SETTING_IQ_DECIMATION, stage_to_use);

    set_frequency(d_frequency);

    set_gains();

    client->startStream();

    is_started = true;
}

void SpyServerSource::stop()
{
    if (is_started)
        client->stopStream();
    is_started = false;
}

void SpyServerSource::close()
{
    if (is_open && is_started)
        client->close();
    is_open = false;
}

void SpyServerSource::set_frequency(uint64_t frequency)
{
    if (is_open && is_connected)
    {
        client->setSetting(SPYSERVER_SETTING_IQ_FREQUENCY, frequency);
        logger->debug("Set SpyServer frequency to {:d}", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void SpyServerSource::drawControlUI()
{
    if (is_connected)
    {
        if (is_started)
            style::beginDisabled();
        ImGui::Combo("Samplerate", &selected_samplerate, samplerate_option_str.c_str());
        current_samplerate = available_samplerates[selected_samplerate];
        stage_to_use = selected_samplerate;
        if (is_started)
            style::endDisabled();
    }

    if (is_started)
        style::beginDisabled();

    if (is_connected)
        style::beginDisabled();
    ImGui::InputText("Address", &ip_address);
    ImGui::InputInt("Port", &port);
    if (is_connected)
        style::endDisabled();

    if (!is_connected)
    {
        if (ImGui::Button("Connect"))
        {
            try
            {
                try_connect();
                error = "";
            }
            catch (std::exception &e)
            {
                logger->error("Error connecting to SpyServer {:s}", e.what());
                error = e.what();
            }
        }
    }
    else
    {
        if (ImGui::Button("Disconnect"))
        {
            disconnect();
            return;
        }
    }

    ImGui::SameLine();
    ImGui::TextColored(ImColor(255, 0, 0), "%s", error.c_str());

    if (ImGui::Combo("Depth", &selected_bit_depth, "32\0"
                                                   "16\0"
                                                   "8\0"))
    {
        if (selected_bit_depth == 0)
            bit_depth = 32;
        else if (selected_bit_depth == 1)
            bit_depth = 16;
        else if (selected_bit_depth == 2)
            bit_depth = 8;
    }

    if (is_started)
        style::endDisabled();

    if (is_connected)
    {
        bool gain_changed = false;
        gain_changed |= ImGui::SliderInt("Gain", &gain, 0, client->devInfo.MaximumGainIndex);
        gain_changed |= ImGui::SliderInt("Digital Gain", &digital_gain, 0, client->devInfo.MaximumGainIndex);
        if (gain_changed)
            set_gains();
    }
}

void SpyServerSource::set_samplerate(uint64_t samplerate)
{
    if (!is_connected)
    {
        buffer_samplerate = samplerate;
    }
    else
    {
        for (int i = 0; i < (int)available_samplerates.size(); i++)
        {
            if (samplerate == available_samplerates[i])
            {
                selected_samplerate = i;
                current_samplerate = samplerate;
                stage_to_use = i;
                buffer_samplerate = 0;
                return;
            }
        }

        if (buffer_samplerate == 0)
            throw std::runtime_error("Unspported samplerate : " + std::to_string(samplerate) + "!");
        buffer_samplerate = 0;
    }
}

uint64_t SpyServerSource::get_samplerate()
{
    return current_samplerate;
}

std::vector<dsp::SourceDescriptor> SpyServerSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    results.push_back({"spyserver", "SpyServer", 0});

    return results;
}
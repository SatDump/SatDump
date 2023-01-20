#include "sdrpp_server.h"
#include "imgui/imgui_stdlib.h"

void SDRPPServerSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    ip_address = getValueOrDefault(d_settings["ip_address"], ip_address);
    port = getValueOrDefault(d_settings["port"], port);
    bit_depth = getValueOrDefault(d_settings["bit_depth"], bit_depth);
    compression = getValueOrDefault(d_settings["compression"], compression);
}

nlohmann::json SDRPPServerSource::get_settings()
{
    d_settings["ip_address"] = ip_address;
    d_settings["port"] = port;
    d_settings["bit_depth"] = bit_depth;
    d_settings["compression"] = compression;

    return d_settings;
}

void SDRPPServerSource::open()
{
    // Nothing to do
    is_open = true;
}

void SDRPPServerSource::start()
{
    // if (is_connected)
    //     disconnect();
    if (!is_connected)
        try_connect();

    DSPSampleSource::start();

    set_params();

    client->start();
    convertShouldRun = true;
    convertThread = std::thread(&SDRPPServerSource::convertFunction, this);

    set_frequency(d_frequency);

    is_started = true;
}

void SDRPPServerSource::stop()
{
    convertShouldRun = false;
    if (convertThread.joinable())
        convertThread.join();
    if (is_started)
        client->stop();
    is_started = false;
}

void SDRPPServerSource::close()
{
    if (is_open && is_started)
        client->close();
    is_open = false;
}

void SDRPPServerSource::set_frequency(uint64_t frequency)
{
    if (is_open && is_connected)
    {
        client->setFrequency(frequency);
        logger->debug("Set SDR++ Server frequency to {:d}", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void SDRPPServerSource::drawControlUI()
{

    if (is_connected)
        style::beginDisabled();
    ImGui::InputText("Adress", &ip_address);
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
                logger->error("Error connecting to SDR++ Server {:s}", e.what());
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

    if (ImGui::Combo("Depth", &selected_bit_depth, "8\0"
                                                   "16\0"
                                                   "32\0"))
    {
        if (selected_bit_depth == 0)
            bit_depth = 8;
        else if (selected_bit_depth == 1)
            bit_depth = 16;
        else if (selected_bit_depth == 2)
            bit_depth = 32;

        set_params();
    }

    if (ImGui::Checkbox("Compression##sdrppcompression", &compression))
        set_params();

    if (is_connected)
    {
        ImGui::Separator();
        client->showMenu();
        ImGui::Separator();
    }
}

void SDRPPServerSource::set_samplerate(uint64_t)
{
    logger->warn("Samplerate can't be set by code for SDR++ Server!!!!");
}

uint64_t SDRPPServerSource::get_samplerate()
{
    uint64_t samplerate = 0;
    if (is_connected)
        samplerate = client->getSampleRate();
    else
        samplerate = 0;
    logger->debug("Got samplerate {:d}", samplerate);
    return samplerate;
}

std::vector<dsp::SourceDescriptor> SDRPPServerSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    results.push_back({"sdrpp_server", "SDR++ Server", 0});

    return results;
}
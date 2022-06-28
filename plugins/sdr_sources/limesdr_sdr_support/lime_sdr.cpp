#include "lime_sdr.h"

void LimeSDRSource::set_gains()
{
    limeDevice->SetGain(false, 0, tia_gain, "TIA");
    limeDevice->SetGain(false, 0, lna_gain, "LNA");
    limeDevice->SetGain(false, 0, pga_gain, "PGA");

    logger->debug("Set LimeSDR TIA to {:d}", tia_gain);
    logger->debug("Set LimeSDR LNA to {:d}", lna_gain);
    logger->debug("Set LimeSDR PGA to {:d}", pga_gain);
}

void LimeSDRSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    tia_gain = getValueOrDefault(d_settings["tia_gain"], tia_gain);
    lna_gain = getValueOrDefault(d_settings["lna_gain"], lna_gain);
    pga_gain = getValueOrDefault(d_settings["pga_gain"], pga_gain);

    if (is_open)
    {
        set_gains();
    }
}

nlohmann::json LimeSDRSource::get_settings(nlohmann::json)
{
    d_settings["tia_gain"] = tia_gain;
    d_settings["lna_gain"] = lna_gain;
    d_settings["pga_gain"] = pga_gain;

    return d_settings;
}

void LimeSDRSource::open()
{
    if (!is_open)
        limeDevice = lime::LMS7_Device::CreateDevice(lime::ConnectionRegistry::findConnections()[d_sdr_id]);
    is_open = true;

    limeDevice->Init();

    // Set available samplerates
    for (int i = 1; i < 81; i++)
        available_samplerates.push_back(i * 1e6);

    // Init UI stuff
    samplerate_option_str = "";
    for (uint64_t samplerate : available_samplerates)
        samplerate_option_str += std::to_string(samplerate) + '\0';
}

void LimeSDRSource::start()
{
    DSPSampleSource::start();

    limeDevice->EnableChannel(false, 0, true);
    limeDevice->SetPath(false, 0, 3);

    logger->debug("Set LimeSDR samplerate to " + std::to_string(current_samplerate));
    limeDevice->SetRate(current_samplerate, 0);
    limeDevice->SetLPF(false, 0, true, current_samplerate);

    set_frequency(d_frequency);

    set_gains();

    limeConfig.align = false;
    limeConfig.isTx = false;
    limeConfig.performanceLatency = 0.5;
    limeConfig.bufferLength = 0; // auto
    limeConfig.format = lime::StreamConfig::FMT_FLOAT32;
    limeConfig.channelID = 0;

    limeStreamID = limeDevice->SetupStream(limeConfig);

    if (limeStreamID == 0)
        throw std::runtime_error("Could not open LimeSDR device stream!");

    limeStream = limeStreamID;
    limeStream->Start();
    needs_to_run = true;

    is_started = true;
}

void LimeSDRSource::stop()
{
    needs_to_run = false;
    if (is_started)
    {
        limeStream->Stop();
        limeDevice->DestroyStream(limeStream);
    }
    is_started = false;
}

void LimeSDRSource::close()
{
    if (is_open)
    {
        limeDevice->Reset();
        lime::ConnectionRegistry::freeConnection(limeDevice->GetConnection());
    }
}

void LimeSDRSource::set_frequency(uint64_t frequency)
{
    if (is_open)
    {
        limeDevice->SetFrequency(false, 0, frequency);
        limeDevice->SetClockFreq(LMS_CLOCK_SXR, frequency, 0);
        logger->debug("Set LimeSDR frequency to {:d}", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void LimeSDRSource::drawControlUI()
{
    if (is_started)
        style::beginDisabled();
    ImGui::Combo("Samplerate", &selected_samplerate, samplerate_option_str.c_str());
    current_samplerate = available_samplerates[selected_samplerate];
    if (is_started)
        style::endDisabled();

    // Gain settings
    bool gain_changed = false;
    gain_changed |= ImGui::SliderInt("TIA Gain", &tia_gain, 0, 12);
    gain_changed |= ImGui::SliderInt("LNA Gain", &lna_gain, 0, 30);
    gain_changed |= ImGui::SliderInt("PGA Gain", &pga_gain, 0, 19);

    if (gain_changed)
        set_gains();
}

void LimeSDRSource::set_samplerate(uint64_t samplerate)
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

uint64_t LimeSDRSource::get_samplerate()
{
    return current_samplerate;
}

std::vector<dsp::SourceDescriptor> LimeSDRSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    lms_info_str_t devices[256];
    int cnt = LMS_GetDeviceList(devices);

    for (int i = 0; i < cnt; i++)
    {
        lms_device_t *device = nullptr;
        LMS_Open(&device, devices[i], NULL);
        const lms_dev_info_t *device_info = LMS_GetDeviceInfo(device);
        std::stringstream ss;
        ss << std::hex << device_info->boardSerialNumber;
        LMS_Close(device);
        results.push_back({"limesdr", "LimeSDR " + ss.str(), (uint64_t)i});
    }

    return results;
}
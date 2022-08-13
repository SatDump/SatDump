#include "lime_sdr.h"

#ifdef __ANDROID__
#include "common/dsp_sample_source/android_usb_backend.h"

#define LIMESDR_USB_VID_PID \
    {                       \
        {                   \
            0x0403, 0x601f  \
        }                   \
    }

// Closing the connection on Android crashes.
// So we cache it and never close it, which is
// not an issue as we should never have 2 Limes
/// on Android.
lime::LMS7_Device *limeDevice_android = nullptr;
lime::StreamChannel *limeStreamID_android = nullptr;
#endif

void LimeSDRSource::set_gains()
{
    if (!is_started)
        return;

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

    if (is_started)
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
    is_open = true;

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

    if (!is_started)
    {
        lime::ConnectionHandle handle;
#ifdef __ANDROID__
        int vid, pid;
        std::string path;
        int fd = getDeviceFD(vid, pid, LIMESDR_USB_VID_PID, path);
        if (limeDevice_android == nullptr)
        {
            limeDevice_android = lime::LMS7_Device::CreateDevice_fd(handle, fd);
            limeDevice_android->Init();
        }
        limeDevice = limeDevice_android;
#else
        limeDevice = lime::LMS7_Device::CreateDevice(lime::ConnectionRegistry::findConnections()[d_sdr_id]);
#endif
        if (limeDevice == NULL)
            throw std::runtime_error("Could not open LimeSDR Device!");
#ifndef __ANDROID__
        limeDevice->Init();
#endif
    }

    limeDevice->EnableChannel(false, 0, true);
    limeDevice->SetPath(false, 0, 3);

    limeConfig.align = false;
    limeConfig.isTx = false;
    limeConfig.performanceLatency = 0.5;
    limeConfig.bufferLength = 0; // auto
    limeConfig.format = lime::StreamConfig::FMT_FLOAT32;
    limeConfig.channelID = 0;

    logger->debug("Set LimeSDR samplerate to " + std::to_string(current_samplerate));
    limeDevice->SetRate(current_samplerate, 0);
    limeDevice->SetLPF(false, 0, true, current_samplerate);

    is_started = true;

    set_frequency(d_frequency);

    set_gains();

#ifndef __ANDROID__
    limeStreamID = limeDevice->SetupStream(limeConfig);
#else
    if (limeStreamID_android == nullptr)
        limeStreamID_android = limeDevice->SetupStream(limeConfig);
    limeStreamID = limeStreamID_android;
#endif

    if (limeStreamID == 0)
        throw std::runtime_error("Could not open LimeSDR device stream!");

    limeStream = limeStreamID;
    limeStream->Start();
    needs_to_run = true;
}

void LimeSDRSource::stop()
{
    needs_to_run = false;
    if (is_started)
    {
        limeStream->Stop();
#ifndef __ANDROID__
        limeDevice->DestroyStream(limeStream);
        limeDevice->Reset();
        lime::ConnectionRegistry::freeConnection(limeDevice->GetConnection());
#endif
    }
    is_started = false;
}

void LimeSDRSource::close()
{
    is_open = false;
}

void LimeSDRSource::set_frequency(uint64_t frequency)
{
    if (is_started)
    {
        limeDevice->SetFrequency(false, 0, frequency);
        // limeDevice->SetClockFreq(LMS_CLOCK_SXR, frequency, 0);
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

#ifndef __ANDROID__
    lms_info_str_t devices[256];
    int cnt = LMS_GetDeviceList(devices);

    for (int i = 0; i < cnt; i++)
    {
        lms_device_t *device = nullptr;
        if (LMS_Open(&device, devices[i], NULL) == -1)
            continue;
        const lms_dev_info_t *device_info = LMS_GetDeviceInfo(device);
        std::stringstream ss;
        ss << std::hex << device_info->boardSerialNumber;
        LMS_Close(device);
        results.push_back({"limesdr", "LimeSDR " + ss.str(), (uint64_t)i});
    }
#else
    int vid, pid;
    std::string path;
    if (getDeviceFD(vid, pid, LIMESDR_USB_VID_PID, path) != -1)
        results.push_back({"limesdr", "LimeSDR USB", 0});
#endif

    return results;
}
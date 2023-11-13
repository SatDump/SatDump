#include "lime_sdr.h"

#ifdef __ANDROID__
#include "common/dsp_source_sink/android_usb_backend.h"

const std::vector<DevVIDPID> LIMESDR_USB_VID_PID = {{0x0403, 0x601f}, {0x0403, 0x6108}};

// Closing the connection on Android crashes.
// So we cache it and never close it, which is
// not an issue as we should never have 2 Limes
// on Android.
lime::LMS7_Device *limeDevice_android = nullptr;
lime::StreamChannel *limeStreamID_android = nullptr;
#endif

void LimeSDRSource::set_gains()
{
    if (!is_started)
        return;

    lime::LMS7_Device *lms = (lime::LMS7_Device *)limeDevice;

    if (gain_mode_manual)
    {
        lms->SetGain(false, channel_id, gain_lna, "LNA");
        lms->SetGain(false, channel_id, gain_tia, "TIA");
        lms->SetGain(false, channel_id, gain_pga, "PGA");
        logger->debug("Set LimeSDR (LNA) Gain to %d", gain_lna);
        logger->debug("Set LimeSDR (TIA) Gain to %d", gain_tia);
        logger->debug("Set LimeSDR (PGA) Gain to %d", gain_pga);
    }
    else
    {
        lms->SetGain(false, channel_id, gain, "");
        logger->debug("Set LimeSDR (auto) Gain to %d", gain);
    }
}

void LimeSDRSource::set_others()
{
    if (!is_started)
        return;

    // lime::LMS7_Device *lms = (lime::LMS7_Device *)limeDevice;

    if (manual_bandwidth)
    {
        LMS_SetLPFBW(limeDevice, false, channel_id, bandwidth_widget.get_value());
        LMS_SetLPF(limeDevice, false, channel_id, true);
    }
    else
    {
        LMS_SetLPFBW(limeDevice, false, channel_id, samplerate_widget.get_value());
        LMS_SetLPF(limeDevice, false, channel_id, true);
    }
}

void LimeSDRSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    gain = getValueOrDefault(d_settings["gain"], gain);
    gain_lna = getValueOrDefault(d_settings["lna_gain"], gain_lna);
    gain_tia = getValueOrDefault(d_settings["tia_gain"], gain_tia);
    gain_pga = getValueOrDefault(d_settings["pga_gain"], gain_pga);
    path_id = getValueOrDefault(d_settings["path_id"], path_id);
    manual_bandwidth = getValueOrDefault(d_settings["manual_bw"], manual_bandwidth);
    bandwidth_widget.set_value(getValueOrDefault(d_settings["manual_bw_value"], samplerate_widget.get_value()));
    channel_id = getValueOrDefault(d_settings["channel_id"], channel_id);

    if (is_started)
    {
        set_gains();
        set_others();
    }
}

nlohmann::json LimeSDRSource::get_settings()
{
    d_settings["gain"] = gain;
    d_settings["lna_gain"] = gain_lna;
    d_settings["tia_gain"] = gain_tia;
    d_settings["pga_gain"] = gain_pga;
    d_settings["path_id"] = path_id;
    d_settings["manual_bw"] = manual_bandwidth;
    d_settings["manual_bw_value"] = bandwidth_widget.get_value();
    d_settings["channel_id"] = channel_id;

    return d_settings;
}

void LimeSDRSource::open()
{
    is_open = true;

    // Set available samplerates
    std::vector<double> available_samplerates;
    for (int i = 1; i < 81; i++)
        available_samplerates.push_back(i * 1e6);

    samplerate_widget.set_list(available_samplerates, true);
    bandwidth_widget.set_list(available_samplerates, true);
}

void LimeSDRSource::start()
{
    DSPSampleSource::start();

    uint64_t current_samplerate = samplerate_widget.get_value();

    if (!is_started)
    {
#ifndef __ANDROID__
        lms_info_str_t found_devices[256];
        LMS_GetDeviceList(found_devices);

        limeDevice = NULL;
        LMS_Open(&limeDevice, found_devices[d_sdr_id], NULL);
        int err = LMS_Init(limeDevice);

        // LimeSuite Bug
        if (err)
        {
            LMS_Close(limeDevice);
            LMS_Open(&limeDevice, found_devices[d_sdr_id], NULL);
            err = LMS_Init(limeDevice);
        }

        if (err)
            throw std::runtime_error("Could not open LimeSDR Device!");
#else
        lime::ConnectionHandle handle;
        int vid, pid;
        std::string path;
        int fd = getDeviceFD(vid, pid, LIMESDR_USB_VID_PID, path);
        if (limeDevice_android == nullptr)
        {
            limeDevice_android = lime::LMS7_Device::CreateDevice_fd(handle, fd, pid == 0x6108);
            limeDevice_android->Init();
        }
        limeDevice = limeDevice_android;

        if (limeDevice == NULL)
            throw std::runtime_error("Could not open LimeSDR Device!");
#endif
    }

#ifdef __ANDROID__
    limeDevice->EnableChannel(false, 0, true);
    limeDevice->SetPath(false, 0, path_id);

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

    if (limeStreamID_android == nullptr)
        limeStreamID_android = limeDevice->SetupStream(limeConfig);
    limeStreamID = limeStreamID_android;

    if (limeStreamID == 0)
        throw std::runtime_error("Could not open LimeSDR device stream!");

    limeStream = limeStreamID;
    limeStream->Start();
#else
    LMS_EnableChannel(limeDevice, false, channel_id, true);
    LMS_SetAntenna(limeDevice, false, channel_id, path_id);

    // limeStream.align = false;
    limeStream.isTx = false;
    limeStream.throughputVsLatency = 0.5;
    limeStream.fifoSize = 8192 * 10; // auto
    limeStream.dataFmt = limeStream.LMS_FMT_F32;
    limeStream.channel = channel_id;

    logger->debug("Set LimeSDR samplerate to " + std::to_string(current_samplerate));
    LMS_SetSampleRate(limeDevice, current_samplerate, 0);

    is_started = true;

    set_frequency(d_frequency);

    set_gains();
    set_others();

    LMS_SetupStream(limeDevice, &limeStream);

    LMS_StartStream(&limeStream);
#endif

    thread_should_run = true;
    work_thread = std::thread(&LimeSDRSource::mainThread, this);
}

void LimeSDRSource::stop()
{
    thread_should_run = false;
    logger->info("Waiting for the thread...");
    if (is_started)
        output_stream->stopWriter();
    if (work_thread.joinable())
        work_thread.join();
    logger->info("Thread stopped");
    if (is_started)
    {
#ifndef __ANDROID__
        LMS_StopStream(&limeStream);
        LMS_DestroyStream(limeDevice, &limeStream);
        LMS_EnableChannel(limeDevice, false, channel_id, false);
        LMS_Close(limeDevice);
#else
        limeStream->Stop();
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
#ifndef __ANDROID__
        LMS_SetLOFrequency(limeDevice, false, channel_id, frequency);
#else
        limeDevice->SetFrequency(false, channel_id, frequency);
#endif
        logger->debug("Set LimeSDR frequency to %d", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void LimeSDRSource::drawControlUI()
{
    if (is_started)
        RImGui::beginDisabled();

    samplerate_widget.render();

    // Channel setting
    RImGui::Combo("Channel####limesdrchannel", &channel_id, "Channel 1\0"
                                                            "Channel 2\0");

    RImGui::Combo("Path####limesdrpath", &path_id, "NONE\0"
                                                   "LNAH\0"
                                                   "LNAL\0"
                                                   "LNAW\0");

    if (is_started)
        RImGui::endDisabled();

    // Gain settings
    bool gain_changed = false;

    if (RImGui::RadioButton("Auto", !gain_mode_manual))
    {
        gain_mode_manual = false;
        gain_changed = true;
    }
    RImGui::SameLine();
    if (RImGui::RadioButton("Manual", gain_mode_manual))
    {
        gain_mode_manual = true;
        gain_changed = true;
    }

    if (gain_mode_manual)
    {
        gain_changed |= RImGui::SliderInt("LNA Gain", &gain_lna, 0, 30);
        gain_changed |= RImGui::SliderInt("TIA Gain", &gain_tia, 0, 12);
        gain_changed |= RImGui::SliderInt("PGA Gain", &gain_pga, -12, 19);
    }
    else
    {
        gain_changed |= RImGui::SliderInt("Gain", &gain, 0, 73);
    }

    if (gain_changed)
        set_gains();

    bool bw_update = RImGui::Checkbox("Manual Bandwidth", &manual_bandwidth);
    if (manual_bandwidth)
        bw_update = bw_update || bandwidth_widget.render();
    if (bw_update && is_started)
        set_others();
}

void LimeSDRSource::set_samplerate(uint64_t samplerate)
{
    if (!samplerate_widget.set_value(samplerate, 100e6))
        throw std::runtime_error("Unspported samplerate : " + std::to_string(samplerate) + "!");
}

uint64_t LimeSDRSource::get_samplerate()
{
    return samplerate_widget.get_value();
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
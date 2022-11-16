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

    LMS_SetGaindB(limeDevice, 0, 0, gain);

    logger->debug("Set LimeSDR Gain to {:d}", gain);
}

void LimeSDRSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    gain = getValueOrDefault(d_settings["gain"], gain);

    if (is_started)
    {
        set_gains();
    }
}

nlohmann::json LimeSDRSource::get_settings()
{
    d_settings["gain"] = gain;

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
        samplerate_option_str += formatSamplerateToString(samplerate) + '\0';
}

void LimeSDRSource::start()
{
    DSPSampleSource::start();

    if (!is_started)
    {
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
    }

    LMS_EnableChannel(limeDevice, false, 0, true);
    LMS_SetAntenna(limeDevice, false, 0, 3);

    // limeStream.align = false;
    limeStream.isTx = false;
    limeStream.throughputVsLatency = 0.5;
    limeStream.fifoSize = 8192 * 10; // auto
    limeStream.dataFmt = limeStream.LMS_FMT_F32;
    limeStream.channel = 0;

    logger->debug("Set LimeSDR samplerate to " + std::to_string(current_samplerate));
    LMS_SetSampleRate(limeDevice, current_samplerate, 0);
    LMS_SetLPFBW(limeDevice, false, 0, current_samplerate);
    LMS_SetLPF(limeDevice, false, 0, true);

    is_started = true;

    set_frequency(d_frequency);

    set_gains();

    LMS_SetupStream(limeDevice, &limeStream);

    LMS_StartStream(&limeStream);

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
        LMS_StopStream(&limeStream);
        LMS_DestroyStream(limeDevice, &limeStream);
        LMS_EnableChannel(limeDevice, false, 0, false);
        LMS_Close(limeDevice);
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
        LMS_SetLOFrequency(limeDevice, false, 0, frequency);
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
    gain_changed |= ImGui::SliderInt("Gain", &gain, 0, 73);

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
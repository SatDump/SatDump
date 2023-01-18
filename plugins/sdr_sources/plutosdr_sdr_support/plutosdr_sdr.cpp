#include "plutosdr_sdr.h"
#include "imgui/imgui_stdlib.h"

#ifdef __ANDROID__
#include "common/dsp_source_sink/android_usb_backend.h"

const std::vector<DevVIDPID> PLUTOSDR_USB_VID_PID = {{0x0456, 0xb673}};

extern "C"
{
    struct iio_context *usb_create_context_fd(unsigned int bus, int fd, uint16_t address, uint16_t intrfc);
}
#endif

// Adapted from https://github.com/altillimity/SatDump/pull/111 and SDR++

const char *pluto_gain_mode[] = {"manual", "fast_attack", "slow_attack", "hybrid"};

void PlutoSDRSource::set_gains()
{
    if (is_open && is_started)
    {
        iio_channel_attr_write(iio_device_find_channel(phy, "voltage0", false), "gain_control_mode", pluto_gain_mode[gain_mode]);
        iio_channel_attr_write_longlong(iio_device_find_channel(phy, "voltage0", false), "hardwaregain", round(gain));
        logger->debug("Set PlutoSDR gain to {:d}, mode {:s}", gain, pluto_gain_mode[gain_mode]);
    }
}

void PlutoSDRSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    gain = getValueOrDefault(d_settings["gain"], gain);
    gain_mode = getValueOrDefault(d_settings["gain_mode"], gain_mode);
    ip_address = getValueOrDefault(d_settings["ip_address"], ip_address);
    auto_reconnect = getValueOrDefault(d_settings["auto_reconnect"], auto_reconnect);

    if (is_open && is_started)
        set_gains();
}

nlohmann::json PlutoSDRSource::get_settings()
{
    d_settings["gain"] = gain;
    d_settings["gain_mode"] = gain_mode;
    d_settings["ip_address"] = ip_address;
    d_settings["auto_reconnect"] = auto_reconnect;

    return d_settings;
}

void PlutoSDRSource::open()
{
    if (d_sdr_id != 0)
        is_usb = true;

    if (!is_open) // We do nothing there!
        is_open = true;

    // Get available samplerates
    available_samplerates.clear();
    for (int sr = 1000000; sr <= 20000000; sr += 500000)
        available_samplerates.push_back(sr);

    // Init UI stuff
    samplerate_option_str = "";
    for (uint64_t samplerate : available_samplerates)
        samplerate_option_str += formatSamplerateToString(samplerate) + '\0';
}

void PlutoSDRSource::start()
{
    DSPSampleSource::start();
    sdr_startup();
    start_thread();
}

void PlutoSDRSource::stop()
{
    stop_thread();
    if (is_started)
        iio_context_destroy(ctx);
    is_started = false;
}

void PlutoSDRSource::close()
{
}

void PlutoSDRSource::set_frequency(uint64_t frequency)
{
    if (is_open && is_started)
    {
        iio_channel_attr_write_longlong(iio_device_find_channel(phy, "altvoltage0", true), "frequency", round(frequency));
        logger->debug("Set PlutoSDR frequency to {:d}", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void PlutoSDRSource::drawControlUI()
{
    if (is_started)
        style::beginDisabled();

    ImGui::Combo("Samplerate", &selected_samplerate, samplerate_option_str.c_str());
    current_samplerate = available_samplerates[selected_samplerate];

    if (!is_usb)
    {
        ImGui::InputText("Address", &ip_address);
        ImGui::Checkbox("Auto-Reconnect", &auto_reconnect);
    }

    if (is_started)
        style::endDisabled();

    if (gain_mode == 0)
    {
        // Gain settings
        if (ImGui::SliderInt("Gain", &gain, 0, 76))
            set_gains();
    }

    if (ImGui::Combo("Gain Mode", &gain_mode, "Manual\0Fast Attack\0Slow Attack\0Hybrid\0"))
        set_gains();
}

void PlutoSDRSource::set_samplerate(uint64_t samplerate)
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

    throw std::runtime_error("Unsupported samplerate : " + std::to_string(samplerate) + "!");
}

uint64_t PlutoSDRSource::get_samplerate()
{
    return current_samplerate;
}

std::vector<dsp::SourceDescriptor> PlutoSDRSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

#ifndef __ANDROID__
    results.push_back({"plutosdr", "PlutoSDR IP", 0});

    // Try to find local USB devices
    iio_scan_context *scan_ctx = iio_create_scan_context("usb", 0);
    struct iio_context_info **info;
    ssize_t ret = iio_scan_context_get_info_list(scan_ctx, &info);

    if (ret > 0)
    {
        // Get dev info
        const char *dev_id = iio_context_info_get_uri(info[0]);

        // Parse to something we can store
        uint8_t x1, x2, x3;
        sscanf(dev_id, "usb:%hhd.%hhd.%hhd", &x1, &x2, &x3);

        // Repack to uint64_t
        uint64_t id = x1 << 16 | x2 << 8 | x3;
        std::string dev_str = "PlutoSDR " + std::to_string(x1) + "." + std::to_string(x2) + "." + std::to_string(x3);

        results.push_back({"plutosdr", dev_str, id});
    }

    iio_scan_context_destroy(scan_ctx);
#else
    int vid, pid;
    std::string path;
    if (getDeviceFD(vid, pid, PLUTOSDR_USB_VID_PID, path) != -1)
        results.push_back({"plutosdr", "PlutoSDR USB", 0});
#endif

    return results;
}

void PlutoSDRSource::sdr_startup()
{
#ifndef __ANDROID__
    if (is_usb)
    {
        uint8_t x1 = (d_sdr_id >> 16) & 0xFF;
        uint8_t x2 = (d_sdr_id >> 8) & 0xFF;
        uint8_t x3 = (d_sdr_id >> 0) & 0xFF;

        std::string usbid = std::to_string(x1) + "." + std::to_string(x2) + "." + std::to_string(x3);
        logger->trace("Using PlutoSDR Device at " + usbid);
        ctx = iio_create_context_from_uri(std::string("usb:" + usbid).c_str());
    }
    else
    {
        logger->trace("Using PlutoSDR IP Address " + ip_address);
        ctx = iio_create_context_from_uri(std::string("ip:" + ip_address).c_str());
    }
#else
    int vid, pid;
    std::string path;
    int fd = getDeviceFD(vid, pid, PLUTOSDR_USB_VID_PID, path);
    ctx = usb_create_context_fd(0, fd, 0, 1);
#endif
    if (ctx == NULL)
        throw std::runtime_error("Could not open PlutoSDR device!");
    phy = iio_context_find_device(ctx, "ad9361-phy");
    if (phy == NULL)
    {
        iio_context_destroy(ctx);
        throw std::runtime_error("Could not connect to PlutoSDR PHY!");
    }
    dev = iio_context_find_device(ctx, "cf-ad9361-lpc");
    if (dev == NULL)
    {
        iio_context_destroy(ctx);
        throw std::runtime_error("Could not connect to PlutoSDR device!");
    }

    iio_channel_attr_write_bool(iio_device_find_channel(phy, "altvoltage1", true), "powerdown", true);
    iio_channel_attr_write_bool(iio_device_find_channel(phy, "altvoltage0", true), "powerdown", false);
    iio_channel_attr_write(iio_device_find_channel(phy, "voltage0", false), "rf_port_select", "A_BALANCED");

    logger->debug("Set PlutoSDR samplerate to " + std::to_string(current_samplerate));
    iio_channel_attr_write_longlong(iio_device_find_channel(phy, "voltage0", false), "sampling_frequency", round(current_samplerate));
    ad9361_set_bb_rate(phy, current_samplerate);

    is_started = true;

    set_frequency(d_frequency);
    set_gains();
}

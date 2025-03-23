/*
F5OEO FixMe :
- RX1 and RX2 should be available only if pluto is Rev C
- CS8/CS16 selection shoudl be available only if firmware text contains tezuka
- Not tested with CLI
*/

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
const char *pluto_iq_mode[] = {"cs16", "cs8"};
const char *pluto_rf_input[] = {"rx1", "rx2"};

void PlutoSDRSource::set_rfinput()
{
    if (is_open && is_started)
    {
         uint32_t val = 0;
         
         iio_device_reg_read(phy, 0x00000003, &val);
         val = (val &0x3F)| ((rf_input+1)<<6);
         iio_device_reg_write(phy, 0x00000003, val);
         logger->trace("RFInput %d", rf_input+1l);   
         iio_device_debug_attr_write_longlong(phy,"adi,1rx-1tx-mode-use-rx-num",rf_input+1);
         // WorkAround because ad9361 driver doesnt reflect well rx change with gain
         iio_channel_attr_write(iio_device_find_channel(phy, "voltage0", false), "gain_control_mode", "fast_attack");
         //Update gain to be coherent between 2 channels : Fixme, should be 2 separate gains   
         set_gains();
    } 
}

void PlutoSDRSource::set_gains()
{
    if (is_open && is_started)
    {
        iio_channel_attr_write(iio_device_find_channel(phy, "voltage0", false), "gain_control_mode", pluto_gain_mode[gain_mode]);
        iio_channel_attr_write_longlong(iio_device_find_channel(phy, "voltage0", false), "hardwaregain", round(gain));
        logger->debug("Set PlutoSDR gain to %d, mode %s", gain, pluto_gain_mode[gain_mode]);
    }
}

void PlutoSDRSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    gain = getValueOrDefault(d_settings["gain"], gain);
    gain_mode = getValueOrDefault(d_settings["gain_mode"], gain_mode);
    ip_address = getValueOrDefault(d_settings["ip_address"], ip_address);
    auto_reconnect = getValueOrDefault(d_settings["auto_reconnect"], auto_reconnect);
    iq_mode=getValueOrDefault(d_settings["iq_mode"], iq_mode);
    rf_input=getValueOrDefault(d_settings["rf_input"], rf_input);
    if (is_open && is_started)
    {
        set_rfinput();
        set_gains();
    }
}

nlohmann::json PlutoSDRSource::get_settings()
{
    d_settings["gain"] = gain;
    d_settings["gain_mode"] = gain_mode;
    d_settings["ip_address"] = ip_address;
    d_settings["auto_reconnect"] = auto_reconnect;
    d_settings["iq_mode"] = iq_mode;
    d_settings["rf_input"] = rf_input;
    return d_settings;
}

void PlutoSDRSource::open()
{
      if (d_sdr_id != "0")
        is_usb = true;

    if (!is_open) // We do nothing there!
        is_open = true;

    // Get available samplerates
    std::vector<double> available_samplerates;
    for (int sr = 1000000; sr <= 20000000; sr += 500000)
        available_samplerates.push_back(sr);

    samplerate_widget.set_list(available_samplerates, true);
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
        logger->debug("Set PlutoSDR frequency to %d", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void PlutoSDRSource::drawControlUI()
{
    if (is_started)
        RImGui::beginDisabled();

    samplerate_widget.render();

    if (!is_usb)
    {
        RImGui::InputText("Address", &ip_address);
        RImGui::Checkbox("Auto-Reconnect", &auto_reconnect);
    }
     if (RImGui::Combo("IQ Mode", &iq_mode, "CS16\0CS8\0")) 
     {};

    if (is_started)
        RImGui::endDisabled();

    if (RImGui::Combo("RF input", &rf_input, "rx1\0rx2\0")) 
     {
        set_rfinput();
     };

    if (gain_mode == 0)
    {
        // Gain settings
        if (RImGui::SteppedSliderInt("Gain", &gain, -1, 73))
            set_gains();
    }

    if (RImGui::Combo("Gain Mode", &gain_mode, "Manual\0Fast Attack\0Slow Attack\0Hybrid\0"))
        set_gains();
}

void PlutoSDRSource::set_samplerate(uint64_t samplerate)
{
    if (!samplerate_widget.set_value(samplerate, 61.44e6))
        throw satdump_exception("Unsupported samplerate : " + std::to_string(samplerate) + "!");
}

uint64_t PlutoSDRSource::get_samplerate()
{
    return samplerate_widget.get_value();
}

std::vector<dsp::SourceDescriptor> PlutoSDRSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

#ifndef __ANDROID__
    //results.push_back({"plutosdr", "PlutoSDR IP", "0", false});

    // Try to find local USB devices
    iio_scan_context *scan_ctx = iio_create_scan_context("usb:ip", 0);
    struct iio_context_info **info;
    ssize_t ret = iio_scan_context_get_info_list(scan_ctx, &info);
    
    if (ret > 0)
    for(int contextidx=0;contextidx<ret;contextidx++)
    {
        // Get dev info
        const char *dev_id = iio_context_info_get_uri(info[contextidx]);
        logger->trace("Context %s",dev_id);
        // Parse to something we can store
        uint8_t x1, x2, x3;
        sscanf(dev_id, ":%hhd.%hhd.%hhd", &x1, &x2, &x3);

        //std::string dev_str = "PlutoSDR " + std::to_string(x1) + "." + std::to_string(x2) + "." + std::to_string(x3);
        std::string dev_str = "PlutoSDR " + std::string(dev_id);

        results.push_back({"plutosdr", dev_str, std::string(dev_id)});
    }

    iio_scan_context_destroy(scan_ctx);
#else
    int vid, pid;
    std::string path;
    if (getDeviceFD(vid, pid, PLUTOSDR_USB_VID_PID, path) != -1)
        results.push_back({"plutosdr", "PlutoSDR USB", "0"});
#endif

    return results;
}

void PlutoSDRSource::sdr_startup()
{
    uint64_t current_samplerate = samplerate_widget.get_value();
    
#ifndef __ANDROID__
    if (is_usb)
    {
        logger->trace("Using PlutoSDR Device at " + d_sdr_id);
        ctx = iio_create_context_from_uri(d_sdr_id.c_str());
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
        throw satdump_exception("Could not open PlutoSDR device!");
    phy = iio_context_find_device(ctx, "ad9361-phy");
    if (phy == NULL)
    {
        iio_context_destroy(ctx);
        throw satdump_exception("Could not connect to PlutoSDR PHY!");
    }
    dev = iio_context_find_device(ctx, "cf-ad9361-lpc");
    if (dev == NULL)
    {
        iio_context_destroy(ctx);
        throw satdump_exception("Could not connect to PlutoSDR device!");
    }

    iio_channel_attr_write_bool(iio_device_find_channel(phy, "altvoltage1", true), "powerdown", true);
    iio_channel_attr_write_bool(iio_device_find_channel(phy, "altvoltage0", true), "powerdown", false);
    iio_channel_attr_write(iio_device_find_channel(phy, "voltage0", false), "rf_port_select", "A_BALANCED");

    logger->debug("Set PlutoSDR samplerate to " + std::to_string(current_samplerate));
    iio_channel_attr_write_longlong(iio_device_find_channel(phy, "voltage0", false), "sampling_frequency", round(current_samplerate));
    ad9361_set_bb_rate(phy, current_samplerate);

    is_started = true;

    set_frequency(d_frequency);
    set_rfinput();
    set_gains();
}

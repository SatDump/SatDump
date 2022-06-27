#include "plutosdr_sdr.h"

// Adapted from https://github.com/altillimity/SatDump/pull/111 and SDR++

const char *pluto_gain_mode[] = {"manual", "fast_attack", "slow_attack", "hybrid"};

void PlutoSDRSource::set_gains()
{
    iio_channel_attr_write(iio_device_find_channel(phy, "voltage0", false), "gain_control_mode", pluto_gain_mode[gain_mode]);
    iio_channel_attr_write_longlong(iio_device_find_channel(phy, "voltage0", false), "hardwaregain", round(gain));
    logger->debug("Set PlutoSDR gain to {:d}, mode {:s}", gain, pluto_gain_mode[gain_mode]);
}

void PlutoSDRSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    gain = getValueOrDefault(d_settings["gain"], gain);
    gain_mode = getValueOrDefault(d_settings["gain_mode"], gain_mode);

    if (is_open)
        set_gains();
}

nlohmann::json PlutoSDRSource::get_settings(nlohmann::json)
{
    d_settings["gain"] = gain;
    d_settings["gain_mode"] = gain_mode;

    return d_settings;
}

void PlutoSDRSource::open()
{
    if (!is_open)
    {
        logger->trace("Using PlutoSDR IP Address " + ip_address);
        ctx = iio_create_context_from_uri(std::string("ip:" + ip_address).c_str());
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
    }
    is_open = true;

    // Get available samplerates
    available_samplerates.clear();
    for (int sr = 1000000; sr <= 20000000; sr += 500000)
        available_samplerates.push_back(sr);

    // Init UI stuff
    samplerate_option_str = "";
    for (uint64_t samplerate : available_samplerates)
        samplerate_option_str += std::to_string(samplerate) + '\0';
}

void PlutoSDRSource::start()
{
    DSPSampleSource::start();

    iio_channel_attr_write_bool(iio_device_find_channel(phy, "altvoltage1", true), "powerdown", true);
    iio_channel_attr_write_bool(iio_device_find_channel(phy, "altvoltage0", true), "powerdown", false);
    iio_channel_attr_write(iio_device_find_channel(phy, "voltage0", false), "rf_port_select", "A_BALANCED");

    logger->debug("Set PlutoSDR samplerate to " + std::to_string(current_samplerate));
    iio_channel_attr_write_longlong(iio_device_find_channel(phy, "voltage0", false), "sampling_frequency", round(current_samplerate));
    ad9361_set_bb_rate(phy, current_samplerate);

    set_frequency(d_frequency);
    set_gains();
    start_thread();

    is_started = true;
}

void PlutoSDRSource::stop()
{
    stop_thread();
    is_started = false;
}

void PlutoSDRSource::close()
{
    if (is_open)
        iio_context_destroy(ctx);
}

void PlutoSDRSource::set_frequency(uint64_t frequency)
{
    if (is_open)
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
    if (is_started)
        style::endDisabled();

    // Gain settings
    if (ImGui::SliderInt("Gain", &gain, 0, 76))
        set_gains();

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

    throw std::runtime_error("Unspported samplerate : " + std::to_string(samplerate) + "!");
}

uint64_t PlutoSDRSource::get_samplerate()
{
    return current_samplerate;
}

std::vector<dsp::SourceDescriptor> PlutoSDRSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;
    results.push_back({"plutosdr", "PlutoSDR", 0});
    return results;
}
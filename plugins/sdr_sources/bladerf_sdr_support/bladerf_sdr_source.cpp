#include "bladerf_sdr_source.h"

void BladeRFSource::set_gains()
{
    bladerf_gain_mode gain_mode_cur;
    bladerf_get_gain_mode(bladerf_dev_obj, BLADERF_CHANNEL_RX(channel_id), &gain_mode_cur);
    if (gain_mode_cur != gain_mode)
        bladerf_set_gain_mode(bladerf_dev_obj, BLADERF_CHANNEL_RX(channel_id), (bladerf_gain_mode)gain_mode);

    if (gain_mode == BLADERF_GAIN_MANUAL)
    {
        bladerf_set_gain(bladerf_dev_obj, BLADERF_CHANNEL_RX(channel_id), general_gain);
        logger->debug("Set BladeRF gain to %d", general_gain);
    }
}

void BladeRFSource::set_bias()
{
    if (bladerf_model == 2)
    {
        bladerf_set_bias_tee(bladerf_dev_obj, BLADERF_CHANNEL_RX(channel_id), bias_enabled);
        logger->debug("Set BladeRF bias to %d", (int)bias_enabled);
    }
}

void BladeRFSource::set_others()
{
    if (bladerf_model == 2)
    {
        bladerf_set_pll_enable(bladerf_dev_obj, extclock_enable);
        logger->debug("Set BladeRF External Clock to %d", (int)extclock_enable);
    }

    if (manual_bandwidth)
        bladerf_set_bandwidth(bladerf_dev_obj, BLADERF_CHANNEL_RX(channel_id), std::clamp<uint64_t>(bandwidth_widget.get_value(), bladerf_range_bandwidth->min, bladerf_range_bandwidth->max), NULL);
    else
        bladerf_set_bandwidth(bladerf_dev_obj, BLADERF_CHANNEL_RX(channel_id), std::clamp<uint64_t>(samplerate_widget.get_value(), bladerf_range_bandwidth->min, bladerf_range_bandwidth->max), NULL);

    bladerf_bandwidth bw = 0;
    bladerf_get_bandwidth(bladerf_dev_obj, BLADERF_CHANNEL_RX(channel_id), &bw);
    logger->debug("Set BladeRF BandWidth to %d", bw);
}

void BladeRFSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    gain_mode = getValueOrDefault(d_settings["gain_mode"], gain_mode);
    general_gain = getValueOrDefault(d_settings["general_gain"], general_gain);
    bias_enabled = getValueOrDefault(d_settings["bias"], bias_enabled);
    extclock_enable = getValueOrDefault(d_settings["extclock"], extclock_enable);
    manual_bandwidth = getValueOrDefault(d_settings["manual_bw"], manual_bandwidth);
    bandwidth_widget.set_value(getValueOrDefault(d_settings["manual_bw_value"], samplerate_widget.get_value()));
    channel_id = getValueOrDefault(d_settings["channel_id"], channel_id);

    if (is_open && is_started)
    {
        set_gains();
        set_bias();
        set_others();
    }
}

nlohmann::json BladeRFSource::get_settings()
{
    d_settings["gain_mode"] = gain_mode;
    d_settings["general_gain"] = general_gain;
    d_settings["bias"] = bias_enabled;
    d_settings["extclock"] = extclock_enable;
    d_settings["manual_bw"] = manual_bandwidth;
    d_settings["manual_bw_value"] = bandwidth_widget.get_value();
    d_settings["channel_id"] = channel_id;

    return d_settings;
}

void BladeRFSource::open()
{
    if (!is_open)
    {
        if (devs_list != NULL)
            bladerf_free_device_list(devs_list);

        devs_cnt = bladerf_get_device_list(&devs_list);

        for (int i = 0; i < devs_cnt; i++)
        {
            if (std::string(devs_list[i].serial) == d_sdr_id)
            {
                selected_dev_id = i;
                if (bladerf_open_with_devinfo(&bladerf_dev_obj, &devs_list[selected_dev_id]) != 0)
                    throw satdump_exception("Could not open BladeRF device!");
            }
        }
    }
    is_open = true;

    // Get channel count
    channel_cnt = bladerf_get_channel_count(bladerf_dev_obj, BLADERF_RX);

    // Get the board version
    const char *bladerf_name = bladerf_get_board_name(bladerf_dev_obj);
    if (std::string(bladerf_name) == "bladerf1")
        bladerf_model = 1;
    else if (std::string(bladerf_name) == "bladerf2")
        bladerf_model = 2;
    else
        bladerf_model = 0;

    // Get all ranges
    bladerf_get_sample_rate_range(bladerf_dev_obj, BLADERF_CHANNEL_RX(channel_id), &bladerf_range_samplerate);
    bladerf_get_bandwidth_range(bladerf_dev_obj, BLADERF_CHANNEL_RX(channel_id), &bladerf_range_bandwidth);
    bladerf_get_gain_range(bladerf_dev_obj, BLADERF_CHANNEL_RX(channel_id), &bladerf_range_gain);

    // Get available samplerates
    std::vector<double> available_samplerates;
    available_samplerates.push_back(bladerf_range_samplerate->min);
    for (int i = 1e6; i < bladerf_range_samplerate->max; i += 1e6)
    {
        // logger->trace("BladeRF device has samplerate %d SPS", i);
        available_samplerates.push_back(i);
    }
    available_samplerates.push_back(bladerf_range_samplerate->max);

    samplerate_widget.set_list(available_samplerates, true);
    bandwidth_widget.set_list(available_samplerates, true, "Hz");

    // Close it
    bladerf_close(bladerf_dev_obj);
}

void BladeRFSource::start()
{
    DSPSampleSource::start();

    if (bladerf_open_with_devinfo(&bladerf_dev_obj, &devs_list[selected_dev_id]) != 0)
        throw satdump_exception("Could not open BladeRF device!");

    uint64_t current_samplerate = samplerate_widget.get_value();

#ifdef BLADERF_HAS_WIDEBAND
    if (current_samplerate > 61.44e6)
    {
        is_8bit = true;
        if (bladerf_enable_feature(bladerf_dev_obj, BLADERF_FEATURE_OVERSAMPLE, true) != 0)
            logger->error("Could not set Oversample mode");
        logger->debug("Using BladeRF Wideband mode");
    }
    else
    {
        is_8bit = false;
        if (bladerf_enable_feature(bladerf_dev_obj, BLADERF_FEATURE_DEFAULT, true) != 0)
            logger->error("Could not set Default mode");
        logger->debug("Using BladeRF Default mode");
    }
#endif

    logger->debug("Set BladeRF samplerate to " + std::to_string(current_samplerate));

#ifdef BLADERF_HAS_WIDEBAND
    if (current_samplerate <= 61.44e6)
    {
#endif
        bladerf_set_sample_rate(bladerf_dev_obj, BLADERF_CHANNEL_RX(channel_id), current_samplerate, NULL);
        logger->info("Actual samplerate %d (precise)", current_samplerate);
#ifdef BLADERF_HAS_WIDEBAND
    }
    else
    {
        struct bladerf_rational_rate rational_rate, actual;
        rational_rate.integer = static_cast<uint32_t>(current_samplerate);
        rational_rate.den = 10000;
        rational_rate.num = (current_samplerate - rational_rate.integer) * rational_rate.den;
        bladerf_set_rational_sample_rate(bladerf_dev_obj, BLADERF_CHANNEL_RX(channel_id), &rational_rate, &actual);
        uint64_t actuals = actual.integer + (actual.num / static_cast<double>(actual.den));
        logger->info("Actual samplerate %d (rational)", actuals);
    }
#endif

    if (manual_bandwidth)
        set_others();
    else
        bladerf_set_bandwidth(bladerf_dev_obj, BLADERF_CHANNEL_RX(channel_id), std::clamp<uint64_t>(current_samplerate, bladerf_range_bandwidth->min, bladerf_range_bandwidth->max), NULL);

    // Setup and start streaming
    // sample_buffer_size = calculate_buffer_size_from_samplerate(samplerate_widget.get_value(), 250);
    sample_buffer_size = std::min<int>(current_samplerate / 250, dsp::STREAM_BUFFER_SIZE);
    sample_buffer_size = (sample_buffer_size / 1024) * 1024;
    if (sample_buffer_size < 1024)
        sample_buffer_size = 1024;
    logger->trace("BladeRF Buffer size %d", sample_buffer_size);

#ifdef BLADERF_HAS_WIDEBAND
    bladerf_sync_config(bladerf_dev_obj, BLADERF_RX_X1, is_8bit ? BLADERF_FORMAT_SC8_Q7 : BLADERF_FORMAT_SC16_Q11, 16, sample_buffer_size, 8, 4000);
#else
    bladerf_sync_config(bladerf_dev_obj, BLADERF_RX_X1, BLADERF_FORMAT_SC16_Q11, 16, sample_buffer_size, 8, 4000);
#endif
    bladerf_enable_module(bladerf_dev_obj, BLADERF_CHANNEL_RX(channel_id), true);

    thread_should_run = true;
    work_thread = std::thread(&BladeRFSource::mainThread, this);

    is_started = true;

    set_frequency(d_frequency);

    set_gains();
    set_bias();
    set_others();
}

void BladeRFSource::stop()
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

        bladerf_set_bias_tee(bladerf_dev_obj, BLADERF_CHANNEL_RX(channel_id), false);
        bladerf_enable_module(bladerf_dev_obj, BLADERF_CHANNEL_RX(channel_id), false);
        bladerf_close(bladerf_dev_obj);
    }
    is_started = false;
}

void BladeRFSource::close()
{
}

void BladeRFSource::set_frequency(uint64_t frequency)
{
    if (is_open && is_started)
    {
        bladerf_set_frequency(bladerf_dev_obj, BLADERF_CHANNEL_RX(channel_id), frequency);
        logger->debug("Set BladeRF frequency to %d", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void BladeRFSource::drawControlUI()
{
    if (is_started)
        RImGui::beginDisabled();

    samplerate_widget.render();

    if (channel_cnt > 1)
        RImGui::Combo("Channel", &channel_id, "RX1\0"
                                              "RX2\0");
    if (is_started)
        RImGui::endDisabled();

    // Gain settings
    if (RImGui::Combo("Gain Mode", &gain_mode, "Default\0"
                                               "Manual\0"
                                               "Fast\0"
                                               "Slow\0"
                                               "Hybrid\0") &&
        is_started)
        set_gains();
    if (is_open)
    {
        if (RImGui::SteppedSliderInt("Gain", &general_gain, bladerf_range_gain->min, bladerf_range_gain->max) && is_started)
            set_gains();
    }
    else // Defaults for using UI when device is not attached (config export)
        RImGui::SteppedSliderInt("Gain", &general_gain, 0, 60);

    if (bladerf_model == 2)
    {
        if (RImGui::Checkbox("Bias-Tee", &bias_enabled) && is_started)
            set_bias();

        if (is_started)
            RImGui::beginDisabled();

        if (RImGui::Checkbox("External Clock", &extclock_enable) && is_started)
            set_others();

        if (is_started)
            RImGui::endDisabled();
    }

    bool bw_update = RImGui::Checkbox("Manual Bandwidth", &manual_bandwidth);
    if (manual_bandwidth)
        bw_update = bw_update || bandwidth_widget.render();
    if (bw_update && is_started)
        set_others();
}

void BladeRFSource::set_samplerate(uint64_t samplerate)
{
    if (!samplerate_widget.set_value(samplerate, 61.44e6))
        throw satdump_exception("Unsupported samplerate : " + std::to_string(samplerate) + "!");
}

uint64_t BladeRFSource::get_samplerate()
{
    return samplerate_widget.get_value();
}

std::vector<dsp::SourceDescriptor> BladeRFSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    bladerf_devinfo *devs_list = nullptr;
    int devs_cnt = bladerf_get_device_list(&devs_list);

    for (int i = 0; i < devs_cnt; i++)
    {
        results.push_back({"bladerf", "BladeRF " + std::string(devs_list[i].serial), std::string(devs_list[i].serial)});
    }

    if (devs_list != nullptr && devs_cnt > 0)
        bladerf_free_device_list(devs_list);

    return results;
}

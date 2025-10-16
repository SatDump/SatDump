#include "rtlsdr_sdr.h"

void RtlSdrSource::_rx_callback(unsigned char *buf, uint32_t len, void *ctx)
{
    std::shared_ptr<dsp::stream<complex_t>> stream = *((std::shared_ptr<dsp::stream<complex_t>> *)ctx);
    for (int i = 0; i < (int)len / 2; i++)
    {
        stream->writeBuf[i].real = (buf[i * 2 + 0] - 127.4f) / 128.0f;
        stream->writeBuf[i].imag = (buf[i * 2 + 1] - 127.4f) / 128.0f;
    }
    stream->swap(len / 2);
};

void RtlSdrSource::set_gains()
{
    if (!is_started)
        return;

    int attempts;
    if (changed_agc)
    {
        // AGC Mode
        attempts = 0;
        while (attempts < 20 && rtlsdr_set_agc_mode(rtlsdr_dev_obj, lna_agc_enabled) < 0)
            attempts++;
        if (attempts == 20)
            logger->warn("Unable to set RTL-SDR AGC mode!");
        else if (attempts == 0)
            logger->debug("Set RTL-SDR AGC to %d", (int)lna_agc_enabled);
        else
            logger->debug("Set RTL-SDR AGC to %d (%d attempts!)", (int)lna_agc_enabled, attempts + 1);

        // Tuner gain mode
        int tuner_gain_mode = tuner_agc_enabled ? 0 : 1;
        attempts = 0;
        while (attempts < 20 && rtlsdr_set_tuner_gain_mode(rtlsdr_dev_obj, tuner_gain_mode) < 0)
            attempts++;
        if (attempts == 20)
            logger->warn("Unable to set RTL-SDR Tuner gain mode!");
        else if (attempts == 0)
            logger->debug("Set RTL-SDR Tuner gain mode to %d", tuner_gain_mode);
        else
            logger->debug("Set RTL-SDR Tuner gain mode to %d (%d attempts!)", tuner_gain_mode, attempts + 1);
    }

    // Get nearest supported tuner gain
    auto gain_iterator = std::lower_bound(available_gains.begin(), available_gains.end(), int(display_gain * 10.0f));
    if (gain_iterator == available_gains.end())
        gain_iterator--;

    bool force_gain = changed_agc && !tuner_agc_enabled;
    if (changed_agc)
        changed_agc = false;

    if (tuner_agc_enabled || (!force_gain && *gain_iterator == gain))
        return;

    if (gain_iterator == available_gains.begin())
        gain_step = 1.0f;
    else
        gain_step = (float)(*gain_iterator - *std::prev(gain_iterator)) / 10.0f;

    // Set tuner gain
    attempts = 0;
    gain = *gain_iterator;
    while (attempts < 20 && rtlsdr_set_tuner_gain(rtlsdr_dev_obj, gain) < 0)
        attempts++;
    if (attempts == 20)
        logger->warn("Unable to set RTL-SDR Gain!");
    else if (attempts == 0)
        logger->debug("Set RTL-SDR Gain to %.1f", (float)gain / 10.0f);
    else
        logger->debug("Set RTL-SDR Gain to %f (%d attempts!)", (float)gain / 10.0f, attempts + 1);
}

void RtlSdrSource::set_bias()
{
    if (!is_started)
        return;

    int attempts = 0;
    while (attempts < 20 && rtlsdr_set_bias_tee(rtlsdr_dev_obj, bias_enabled) < 0)
        attempts++;
    if (attempts == 20)
        logger->warn("Unable to set RTL-SDR Bias!");
    else if (attempts == 0)
        logger->debug("Set RTL-SDR Bias to %d", (int)bias_enabled);
    else
        logger->debug("Set RTL-SDR Bias to %d (%d attempts!)", (int)bias_enabled, attempts + 1);
}

void RtlSdrSource::set_ppm()
{
    int ppm = ppm_widget.get();
    if (!is_started || ppm == last_ppm)
        return;

    last_ppm = ppm;
    int attempts = 0;
    while (attempts < 20 && rtlsdr_set_freq_correction(rtlsdr_dev_obj, ppm) < 0)
        attempts++;
    if (attempts == 20)
        logger->warn("Unable to set RTL-SDR PPM Correction!");
    else if (attempts == 0)
        logger->debug("Set RTL-SDR PPM Correction to %d", ppm);
    else
        logger->debug("Set RTL-SDR PPM Correction to %d (%d attempts!)", ppm, attempts + 1);
}

void RtlSdrSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    // Convert legacy AGC setting to new
    if (d_settings.contains("agc"))
    {
        lna_agc_enabled = tuner_agc_enabled = getValueOrDefault(d_settings["agc"], false);
        d_settings.erase("agc");
    }

    display_gain = getValueOrDefault(d_settings["gain"], display_gain);
    lna_agc_enabled = getValueOrDefault(d_settings["lna_agc"], lna_agc_enabled);
    tuner_agc_enabled = getValueOrDefault(d_settings["tuner_agc"], tuner_agc_enabled);
    bias_enabled = getValueOrDefault(d_settings["bias"], bias_enabled);
    ppm_widget.set(getValueOrDefault(d_settings["ppm_correction"], ppm_widget.get()));
    changed_agc = true;

    if (is_started)
    {
        set_bias();
        set_gains();
        set_ppm();
    }
}

nlohmann::json RtlSdrSource::get_settings()
{
    d_settings["gain"] = display_gain;
    d_settings["lna_agc"] = lna_agc_enabled;
    d_settings["tuner_agc"] = tuner_agc_enabled;
    d_settings["bias"] = bias_enabled;
    d_settings["ppm_correction"] = ppm_widget.get();

    return d_settings;
}

void RtlSdrSource::open()
{
    is_open = true;

    // Set available samplerate
    std::vector<double> available_samplerates;
    available_samplerates.push_back(250000);
    available_samplerates.push_back(1024000);
    available_samplerates.push_back(1536000);
    available_samplerates.push_back(1792000);
    available_samplerates.push_back(1920000);
    available_samplerates.push_back(2048000);
    available_samplerates.push_back(2160000);
    available_samplerates.push_back(2400000);
    available_samplerates.push_back(2560000);
    available_samplerates.push_back(2880000);
    available_samplerates.push_back(3200000);

    samplerate_widget.set_list(available_samplerates, true);
}

void RtlSdrSource::start()
{
    DSPSampleSource::start();

    int index = rtlsdr_get_index_by_serial(d_sdr_id.c_str());
    if (index != -1 && rtlsdr_open(&rtlsdr_dev_obj, index) != 0)
        throw satdump_exception("Could not open RTL-SDR device!");

    // Set available gains
    int gains[256];
    int num_gains = rtlsdr_get_tuner_gains(rtlsdr_dev_obj, gains);
    if (num_gains > 0)
    {
        available_gains.clear();
        for (int i = 0; i < num_gains; i++)
            available_gains.push_back(gains[i]);
        std::sort(available_gains.begin(), available_gains.end());
    }

    uint64_t current_samplerate = samplerate_widget.get_value();

    logger->debug("Set RTL-SDR samplerate to " + std::to_string(current_samplerate));
    rtlsdr_set_sample_rate(rtlsdr_dev_obj, current_samplerate);

    is_started = true;
    changed_agc = true;

    set_frequency(d_frequency);

    set_bias();
    set_gains();
    set_ppm();

    rtlsdr_reset_buffer(rtlsdr_dev_obj);
    display_gain = (float)gain / 10.0f;
    thread_should_run = true;
    work_thread = std::thread(&RtlSdrSource::mainThread, this);
}

void RtlSdrSource::stop()
{
    if (is_started)
    {
        rtlsdr_cancel_async(rtlsdr_dev_obj);
        thread_should_run = false;
        logger->info("Waiting for the thread...");
        if (is_started)
            output_stream->stopWriter();
        if (work_thread.joinable())
            work_thread.join();
        logger->info("Thread stopped");
        rtlsdr_set_bias_tee(rtlsdr_dev_obj, false);
        rtlsdr_close(rtlsdr_dev_obj);
    }
    is_started = false;
}

void RtlSdrSource::close() { is_open = false; }

void RtlSdrSource::set_frequency(uint64_t frequency)
{
    if (is_started)
    {
        int attempts = 0;
        while (attempts < 20 && rtlsdr_set_center_freq(rtlsdr_dev_obj, frequency) < 0)
            attempts++;
        if (attempts == 20)
            logger->warn("Unable to set RTL-SDR frequency!");
        else if (attempts == 0)
        {
            logger->debug("Set RTL-SDR frequency to %d", frequency);

            // This is a dirty patch! PLL might not lock on the first attempt if the frequency
            // is above ~1 GHz, tuning down then back up locks it! This is a librtlsdr bug,
            // because the return code should show that it wasn't successful, but it doesn't.
            if (frequency > 1e6)
            {
                rtlsdr_set_center_freq(rtlsdr_dev_obj, frequency - 1e6);
                rtlsdr_set_center_freq(rtlsdr_dev_obj, frequency);
            }
        }
        else
            logger->debug("Set RTL-SDR frequency to %d (%d attempts!)", frequency, attempts + 1);
    }
    DSPSampleSource::set_frequency(frequency);
}

void RtlSdrSource::drawControlUI()
{
    if (is_started)
        RImGui::beginDisabled();

    samplerate_widget.render();

    if (is_started)
        RImGui::endDisabled();

    if (ppm_widget.draw())
        set_ppm();

    if (tuner_agc_enabled)
        RImGui::beginDisabled();
    if (RImGui::SteppedSliderFloat("Tuner Gain", &display_gain, (float)available_gains[0] / 10.0f, (float)available_gains.back() / 10.0f, gain_step, "%.1f"))
        set_gains();
    if (is_started && RImGui::IsItemDeactivatedAfterEdit())
        display_gain = (float)gain / 10.0f;
    if (tuner_agc_enabled)
        RImGui::endDisabled();

    if (RImGui::Checkbox("LNA AGC", &lna_agc_enabled))
    {
        changed_agc = true;
        set_gains();
    }

    if (RImGui::Checkbox("Tuner AGC", &tuner_agc_enabled))
    {
        changed_agc = true;
        set_gains();
    }

    if (RImGui::Checkbox("Bias-Tee", &bias_enabled))
        set_bias();
}

void RtlSdrSource::set_samplerate(uint64_t samplerate)
{
    if (!samplerate_widget.set_value(samplerate, 3.2e6))
        throw satdump_exception("Unsupported samplerate : " + std::to_string(samplerate) + "!");
}

uint64_t RtlSdrSource::get_samplerate() { return samplerate_widget.get_value(); }

std::vector<dsp::SourceDescriptor> RtlSdrSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    int c = rtlsdr_get_device_count();

    for (int i = 0; i < c; i++)
    {
        const char *name = rtlsdr_get_device_name(i);
        char manufact[256], product[256], serial[256];
        if (rtlsdr_get_device_usb_strings(i, manufact, product, serial) != 0)
            results.push_back({"rtlsdr", std::string(name) + " #" + std::to_string(i), std::to_string(i)});
        else
            results.push_back({"rtlsdr", std::string(manufact) + " " + std::string(product) + " #" + std::string(serial), std::string(serial)});
    }

    return results;
}

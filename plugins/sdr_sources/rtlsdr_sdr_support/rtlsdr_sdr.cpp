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

void RtlSdrSource::set_gain(std::vector<int> available_gain, float setgain, bool changed_agc, bool tuner_agc_enabled, bool e4000, int e4000_stage)
{
    // Get nearest supported tuner gain
    auto gain_iterator = std::lower_bound(available_gain.begin(), available_gain.end(), int(setgain * 10.0f));
    if (gain_iterator == available_gain.end())
        gain_iterator--;

    if (!e4000) {
        bool force_gain = changed_agc && !tuner_agc_enabled;
        if (tuner_agc_enabled || (!force_gain && *gain_iterator == setgain))
            return;

        if (gain_iterator == available_gain.begin())
            gain_step = 1.0f;
        else
            gain_step = (float)(*gain_iterator - *std::prev(gain_iterator)) / 10.0f;
    }

    // Set gain
    int attempts = 0;
    gain[e4000_stage] = *gain_iterator;
    while (attempts < 20 && (e4000 ? rtlsdr_set_tuner_if_gain(rtlsdr_dev_obj, e4000_stage, gain[e4000_stage]) :
                                     rtlsdr_set_tuner_gain(rtlsdr_dev_obj, gain[e4000_stage])) < 0)
        attempts++;
    if (attempts == 20)
        logger->warn("Unable to set RTL-SDR Gain [E4000=%d stage=%d]!", e4000, e4000_stage);
    else if (attempts == 0)
        logger->debug("Set RTL-SDR Gain [E4000=%d stage=%d] to %.1f", e4000, e4000_stage, (float)gain[e4000_stage] / 10.0f);
    else
        logger->debug("Set RTL-SDR Gain [E4000=%d stage=%d] to %f (%d attempts!)", e4000, e4000_stage, (float)gain[e4000_stage] / 10.0f, attempts + 1);
}

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

    set_gain(available_gains, display_gain, changed_agc, tuner_agc_enabled, false, 0);
    if (tuner_is_e4000) {
        for (int i = 0; i < 6; i++)
            set_gain(available_gains_e4000[i], display_gain_e4000[i], false, false, true, i + 1);
    }

    if (changed_agc)
        changed_agc = false;
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
    display_gain_e4000[0] = getValueOrDefault(d_settings["gain_e4000_s1"], display_gain_e4000[0]);
    display_gain_e4000[1] = getValueOrDefault(d_settings["gain_e4000_s2"], display_gain_e4000[1]);
    display_gain_e4000[2] = getValueOrDefault(d_settings["gain_e4000_s3"], display_gain_e4000[2]);
    display_gain_e4000[3] = getValueOrDefault(d_settings["gain_e4000_s4"], display_gain_e4000[3]);
    display_gain_e4000[4] = getValueOrDefault(d_settings["gain_e4000_s5"], display_gain_e4000[4]);
    display_gain_e4000[5] = getValueOrDefault(d_settings["gain_e4000_s6"], display_gain_e4000[5]);
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
    d_settings["gain_e4000_s1"] = display_gain_e4000[0];
    d_settings["gain_e4000_s2"] = display_gain_e4000[1];
    d_settings["gain_e4000_s3"] = display_gain_e4000[2];
    d_settings["gain_e4000_s4"] = display_gain_e4000[3];
    d_settings["gain_e4000_s5"] = display_gain_e4000[4];
    d_settings["gain_e4000_s6"] = display_gain_e4000[5];
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

    tuner_is_e4000 = (rtlsdr_get_tuner_type(rtlsdr_dev_obj) == RTLSDR_TUNER_E4000);

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
    display_gain = (float)gain[0] / 10.0f;
    for (int i = 0; i < 6; i++)
        display_gain_e4000[i] = (float)gain[i + 1] / 10.0f;
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

void RtlSdrSource::close()
{
    is_open = false;
}

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
            logger->debug("Set RTL-SDR frequency to %d", frequency);
        else
            logger->debug("Set RTL-SDR frequency to %d (%d attempts!)", frequency, attempts + 1);
    }
    DSPSampleSource::set_frequency(frequency);
}

void RtlSdrSource::drawControlUI()
{
    bool update_gains = false;
    bool refresh_display_gain[7] = { false, false, false, false, false, false, false };

    if (is_started)
        RImGui::beginDisabled();

    samplerate_widget.render();

    if (is_started)
        RImGui::endDisabled();

    if (ppm_widget.draw())
        set_ppm();

    if (tuner_agc_enabled)
        RImGui::beginDisabled();
    if (RImGui::SteppedSliderFloat("Tuner Gain", &display_gain, (float)available_gains[0] / 10.0f,
        (float)available_gains.back() / 10.0f, gain_step, "%.1f"))
            update_gains = true;
    refresh_display_gain[0] = is_started && RImGui::IsItemDeactivatedAfterEdit();

    if (tuner_is_e4000) {
        /* See e.g. https://k3xec.com/e4k/ for E4000 gain setting details */
        if (RImGui::SteppedSliderFloat("E4000 IF S1 Gain", &(display_gain_e4000[0]),
                                       (float)available_gains_e4000[0][0] / 10.0f,
                                       (float)available_gains_e4000[0].back() / 10.0f,
                                       9.0, "%.1f"))
                update_gains = true;
        refresh_display_gain[1] = is_started && RImGui::IsItemDeactivatedAfterEdit();

        if (RImGui::SteppedSliderFloat("E4000 IF S2 Gain", &(display_gain_e4000[1]),
                                       (float)available_gains_e4000[1][0] / 10.0f,
                                       (float)available_gains_e4000[1].back() / 10.0f,
                                       3.0, "%.1f"))
                update_gains = true;
        refresh_display_gain[2] = is_started && RImGui::IsItemDeactivatedAfterEdit();

        if (RImGui::SteppedSliderFloat("E4000 IF S3 Gain", &(display_gain_e4000[2]),
                                       (float)available_gains_e4000[2][0] / 10.0f,
                                       (float)available_gains_e4000[2].back() / 10.0f,
                                       3.0, "%.1f"))
                update_gains = true;
        refresh_display_gain[3] = is_started && RImGui::IsItemDeactivatedAfterEdit();

        if (RImGui::SteppedSliderFloat("E4000 IF S4 Gain", &(display_gain_e4000[3]),
                                       (float)available_gains_e4000[3][0] / 10.0f,
                                       (float)available_gains_e4000[3].back() / 10.0f,
                                       1.0, "%.1f"))
                update_gains = true;
        refresh_display_gain[4] = is_started && RImGui::IsItemDeactivatedAfterEdit();

        if (RImGui::SteppedSliderFloat("E4000 IF S5 Gain", &(display_gain_e4000[4]),
                                       (float)available_gains_e4000[4][0] / 10.0f,
                                       (float)available_gains_e4000[4].back() / 10.0f,
                                       3.0, "%.1f"))
                update_gains = true;
        refresh_display_gain[5] = is_started && RImGui::IsItemDeactivatedAfterEdit();

        if (RImGui::SteppedSliderFloat("E4000 IF S6 Gain", &(display_gain_e4000[5]),
                                       (float)available_gains_e4000[5][0] / 10.0f,
                                       (float)available_gains_e4000[5].back() / 10.0f,
                                       3.0, "%.1f"))
                update_gains = true;
        refresh_display_gain[6] = is_started && RImGui::IsItemDeactivatedAfterEdit();
    }

    if (tuner_agc_enabled)
        RImGui::endDisabled();

    if (RImGui::Checkbox("LNA AGC", &lna_agc_enabled))
    {
        changed_agc = true;
        update_gains = true;
    }

    if (RImGui::Checkbox("Tuner AGC", &tuner_agc_enabled))
    {
        changed_agc = true;
        update_gains = true;
    }

    if (update_gains)
        set_gains();

    if (RImGui::Checkbox("Bias-Tee", &bias_enabled))
        set_bias();

    if (refresh_display_gain[0])
        display_gain = (float)gain[0] / 10.0f;
    for (int i = 0; i < 6; i++)
        if (refresh_display_gain[i + 1])
            display_gain_e4000[i] = (float)gain[i + 1] / 10.0f;
}

void RtlSdrSource::set_samplerate(uint64_t samplerate)
{
    if (!samplerate_widget.set_value(samplerate, 3.2e6))
        throw satdump_exception("Unsupported samplerate : " + std::to_string(samplerate) + "!");
}

uint64_t RtlSdrSource::get_samplerate()
{
    return samplerate_widget.get_value();
}

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

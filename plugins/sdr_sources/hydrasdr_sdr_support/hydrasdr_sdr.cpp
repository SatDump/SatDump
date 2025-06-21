#include "hydrasdr_sdr.h"

int HydraSDRSource::_rx_callback(hydrasdr_transfer *t)
{
    std::shared_ptr<dsp::stream<complex_t>> stream = *((std::shared_ptr<dsp::stream<complex_t>> *)t->ctx);
    memcpy(stream->writeBuf, t->samples, t->sample_count * sizeof(complex_t));
    stream->swap(t->sample_count);
    return 0;
}

void HydraSDRSource::set_gains()
{
    if (!is_started)
        return;

    if (gain_type == 0)
    {
        hydrasdr_set_sensitivity_gain(hydrasdr_dev_obj, general_gain);
        logger->debug("Set HydraSDR gain (sensitive) to %d", general_gain);
    }
    else if (gain_type == 1)
    {
        hydrasdr_set_linearity_gain(hydrasdr_dev_obj, general_gain);
        logger->debug("Set HydraSDR gain (linear) to %d", general_gain);
    }
    else if (gain_type == 2)
    {
        hydrasdr_set_lna_gain(hydrasdr_dev_obj, manual_gains[0]);
        hydrasdr_set_mixer_gain(hydrasdr_dev_obj, manual_gains[1]);
        hydrasdr_set_vga_gain(hydrasdr_dev_obj, manual_gains[2]);
        logger->debug("Set HydraSDR gain (manual) to %d, %d, %d", manual_gains[0], manual_gains[1], manual_gains[2]);
    }
}

void HydraSDRSource::set_bias()
{
    if (!is_started)
        return;

    hydrasdr_set_rf_bias(hydrasdr_dev_obj, bias_enabled);
    logger->debug("Set HydraSDR bias to %d", (int)bias_enabled);
}

void HydraSDRSource::set_agcs()
{
    if (!is_started)
        return;

    hydrasdr_set_lna_agc(hydrasdr_dev_obj, lna_agc_enabled);
    hydrasdr_set_mixer_agc(hydrasdr_dev_obj, mixer_agc_enabled);
    logger->debug("Set HydraSDR LNA AGC to %d", (int)lna_agc_enabled);
    logger->debug("Set HydraSDR Mixer AGC to %d", (int)mixer_agc_enabled);
}

void HydraSDRSource::open_sdr()
{
    if (hydrasdr_open_sn(&hydrasdr_dev_obj, std::stoull(d_sdr_id)) != HYDRASDR_SUCCESS)
        throw satdump_exception("Could not open HydraSDR device!");
}

void HydraSDRSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    gain_type = getValueOrDefault(d_settings["gain_type"], gain_type);
    general_gain = getValueOrDefault(d_settings["general_gain"], general_gain);
    manual_gains[0] = getValueOrDefault(d_settings["lna_gain"], manual_gains[0]);
    manual_gains[1] = getValueOrDefault(d_settings["mixer_gain"], manual_gains[1]);
    manual_gains[2] = getValueOrDefault(d_settings["vga_gain"], manual_gains[2]);

    bias_enabled = getValueOrDefault(d_settings["bias"], bias_enabled);
    lna_agc_enabled = getValueOrDefault(d_settings["lna_agc"], lna_agc_enabled);
    mixer_agc_enabled = getValueOrDefault(d_settings["mixer_agc"], mixer_agc_enabled);

    if (is_started)
    {
        set_gains();
        set_bias();
        set_agcs();
    }
}

nlohmann::json HydraSDRSource::get_settings()
{
    d_settings["gain_type"] = gain_type;
    d_settings["general_gain"] = general_gain;
    d_settings["lna_gain"] = manual_gains[0];
    d_settings["mixer_gain"] = manual_gains[1];
    d_settings["vga_gain"] = manual_gains[2];

    d_settings["bias"] = bias_enabled;
    d_settings["lna_agc"] = lna_agc_enabled;
    d_settings["mixer_agc"] = mixer_agc_enabled;

    return d_settings;
}

void HydraSDRSource::open()
{
    open_sdr();
    is_open = true;

    // Get available samplerates
    uint32_t samprate_cnt;
    uint32_t dev_samplerates[10];
    hydrasdr_get_samplerates(hydrasdr_dev_obj, &samprate_cnt, 0);
    hydrasdr_get_samplerates(hydrasdr_dev_obj, dev_samplerates, samprate_cnt);
    std::vector<double> available_samplerates;
    bool has_10msps = false;
    for (int i = samprate_cnt - 1; i >= 0; i--)
    {
        logger->trace("HydraSDR device has samplerate %d SPS", dev_samplerates[i]);
        available_samplerates.push_back(dev_samplerates[i]);
        if (dev_samplerates[i] == 10e6)
            has_10msps = true;
    }

    if (!has_10msps)
        available_samplerates.push_back(10e6);

    samplerate_widget.set_list(available_samplerates, false);

    hydrasdr_close(hydrasdr_dev_obj);
}

void HydraSDRSource::start()
{
    DSPSampleSource::start();
    open_sdr();

    uint64_t current_samplerate = samplerate_widget.get_value();

    hydrasdr_set_sample_type(hydrasdr_dev_obj, HYDRASDR_SAMPLE_FLOAT32_IQ);

    logger->debug("Set HydraSDR samplerate to " + std::to_string(current_samplerate));
    hydrasdr_set_samplerate(hydrasdr_dev_obj, current_samplerate);

    is_started = true;

    set_frequency(d_frequency);

    set_gains();
    set_bias();
    set_agcs();

    hydrasdr_set_packing(hydrasdr_dev_obj, 1);
    hydrasdr_start_rx(hydrasdr_dev_obj, &_rx_callback, &output_stream);
}

void HydraSDRSource::stop()
{
    if (is_started)
    {
        hydrasdr_set_rf_bias(hydrasdr_dev_obj, false);
        hydrasdr_stop_rx(hydrasdr_dev_obj);
        hydrasdr_close(hydrasdr_dev_obj);
    }
    is_started = false;
}

void HydraSDRSource::close() {}

void HydraSDRSource::set_frequency(uint64_t frequency)
{
    if (is_started)
    {
        hydrasdr_set_freq(hydrasdr_dev_obj, frequency);
        logger->debug("Set HydraSDR frequency to %d", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void HydraSDRSource::drawControlUI()
{
    if (is_started)
        RImGui::beginDisabled();

    samplerate_widget.render();

    if (is_started)
        RImGui::endDisabled();

    // Gain settings
    bool gain_changed = false;
    if (RImGui::RadioButton("Sensitive", gain_type == 0))
    {
        gain_type = 0;
        gain_changed = true;
    }
    if (RImGui::RadioButton("Linear", gain_type == 1))
    {
        gain_type = 1;
        gain_changed = true;
    }
    if (RImGui::RadioButton("Manual", gain_type == 22))
    {
        gain_type = 2;
        gain_changed = true;
    }

    if (gain_type == 2)
    {
        gain_changed |= RImGui::SteppedSliderInt("LNA Gain", &manual_gains[0], 0, 15);
        gain_changed |= RImGui::SteppedSliderInt("Mixer Gain", &manual_gains[1], 0, 15);
        gain_changed |= RImGui::SteppedSliderInt("VGA Gain", &manual_gains[2], 0, 15);
    }
    else
    {
        gain_changed |= RImGui::SteppedSliderInt("Gain", &general_gain, 0, 21);
    }

    if (gain_changed)
        set_gains();

    if (RImGui::Checkbox("Bias-Tee", &bias_enabled))
        set_bias();

    if (RImGui::Checkbox("LNA AGC", &lna_agc_enabled))
        set_agcs();

    if (RImGui::Checkbox("Mixer AGC", &mixer_agc_enabled))
        set_agcs();
}

void HydraSDRSource::set_samplerate(uint64_t samplerate)
{
    if (!samplerate_widget.set_value(samplerate, 10e6))
        throw satdump_exception("Unsupported samplerate : " + std::to_string(samplerate) + "!");
}

uint64_t HydraSDRSource::get_samplerate() { return samplerate_widget.get_value(); }

std::vector<dsp::SourceDescriptor> HydraSDRSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    uint64_t serials[100];
    int c = hydrasdr_list_devices(serials, 100);

    for (int i = 0; i < c; i++)
    {
        std::stringstream ss;
        ss << std::hex << serials[i];
        results.push_back({"hydrasdr", "HydraSDR One " + ss.str(), std::to_string(serials[i])});
    }

    return results;
}

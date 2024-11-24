#include "sddc_sdr.h"
#include "resources.h"

/*
I am totally not sure if any of the control stuff or Gain / etc is done properly,
but with the very limited documentation and SDDC API that was already a pain to
get running at all, at least it seems to work with the RX888. Untested with others.

Not even sure I'm doing samplerate stuff correctly either :-)
*/

complex_t SDDCSource::ddc_phase_delta = complex_t(cos(-M_PI * 0.5), sin(-M_PI * 0.5));
complex_t SDDCSource::ddc_phase = complex_t(1, 0);

void SDDCSource::_rx_callback(uint32_t data_size, const float *data, void *context)
{
    std::shared_ptr<dsp::stream<complex_t>> stream = *((std::shared_ptr<dsp::stream<complex_t>> *)context);
    volk_32fc_s32fc_x2_rotator_32fc((lv_32fc_t *)stream->writeBuf, (lv_32fc_t *)data, SDDCSource::ddc_phase_delta, (lv_32fc_t *)&SDDCSource::ddc_phase, data_size / 2);
    stream->swap(data_size / 2);
}

void SDDCSource::set_att()
{
    if (is_started)
        sddc_stop_streaming(sddc_dev_obj);
    sddc_set_tuner_rf_attenuation(sddc_dev_obj, rf_gain);
    sddc_set_tuner_if_attenuation(sddc_dev_obj, if_gain);
    if (is_started)
        sddc_start_streaming(sddc_dev_obj);
}

void SDDCSource::set_bias()
{
    if (is_started)
        sddc_stop_streaming(sddc_dev_obj);
    if (mode == 0)
        sddc_set_hf_bias(sddc_dev_obj, bias);
    else if (mode == 0)
        sddc_set_vhf_bias(sddc_dev_obj, bias);
    if (is_started)
        sddc_start_streaming(sddc_dev_obj);
}

void SDDCSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    mode = getValueOrDefault(d_settings["mode"], mode);
    rf_gain = getValueOrDefault(d_settings["rf_gain"], rf_gain);
    if_gain = getValueOrDefault(d_settings["if_gain"], if_gain);
    bias = getValueOrDefault(d_settings["bias"], bias);
}

nlohmann::json SDDCSource::get_settings()
{
    d_settings["mode"] = mode;
    d_settings["rf_gain"] = rf_gain;
    d_settings["if_gain"] = if_gain;
    d_settings["bias"] = bias;

    return d_settings;
}

void SDDCSource::open()
{
    if (!is_open)
    {
        sddc_dev_obj = sddc_open(std::stoi(d_sdr_id), resources::getResourcePath("sddc/SDDC_FX3.img").c_str());
        if (sddc_dev_obj == NULL)
            throw satdump_exception("Could not open SDDC device!");
    }
    is_open = true;

    // Get available samplerates
    available_samplerates.clear();
    available_samplerates.push_back(16e6);
    available_samplerates.push_back(32e6);
    available_samplerates.push_back(64e6);
    available_samplerates.push_back(128e6);

    // Init UI stuff
    samplerate_option_str = "";
    for (uint64_t samplerate : available_samplerates)
        samplerate_option_str += format_notated(samplerate / 2, "sps") + '\0';
}

void SDDCSource::start()
{
    DSPSampleSource::start();

    logger->debug("Set SDDC samplerate to " + std::to_string(current_samplerate));
    sddc_set_sample_rate(sddc_dev_obj, current_samplerate);

    if (mode == 1)
        sddc_set_tuner_bw(sddc_dev_obj, current_samplerate);

    sddc_set_rf_mode(sddc_dev_obj, mode ? VHF_MODE : HF_MODE);
    sddc_set_async_params(sddc_dev_obj, 0, 0, _rx_callback, &sddc_stream);

    set_frequency(d_frequency);

    set_att();
    set_bias();

    sddc_stream = std::make_shared<dsp::stream<complex_t>>();
    resampler = std::make_shared<dsp::RationalResamplerBlock<complex_t>>(sddc_stream, 1, 2);
    output_stream = resampler->output_stream;
    resampler->start();

    sddc_start_streaming(sddc_dev_obj);

    is_started = true;
}

void SDDCSource::stop()
{
    sddc_stop_streaming(sddc_dev_obj);
    sddc_handle_events(sddc_dev_obj);
    resampler->stop();
    is_started = false;
}

void SDDCSource::close()
{
    if (is_open)
        sddc_close(sddc_dev_obj);
}

void SDDCSource::set_frequency(uint64_t frequency)
{
    if (is_open)
    {
        if (mode == 1)
        {
            if (is_started)
                sddc_stop_streaming(sddc_dev_obj);
            sddc_set_tuner_frequency(sddc_dev_obj, frequency);
            if (is_started)
                sddc_start_streaming(sddc_dev_obj);
        }
        logger->debug("Set SDDC frequency to %d", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void SDDCSource::drawControlUI()
{
    if (is_started)
        RImGui::beginDisabled();

    RImGui::Combo("Samplerate", &selected_samplerate, samplerate_option_str.c_str());
    current_samplerate = available_samplerates[selected_samplerate];

    RImGui::Combo("Mode", &mode, "HF\0"
                                 "VHF\0");

    if (is_started)
        RImGui::endDisabled();

    // Gain settings
    bool gain_changed = false;
    if (mode == 1)
    {
        gain_changed |= RImGui::SteppedSliderInt("RF Gain", &rf_gain, 0, 29);
        gain_changed |= RImGui::SteppedSliderInt("IF Gain", &if_gain, 0, 16);
    }
    else
    {
        gain_changed |= RImGui::SteppedSliderInt("RF Gain", &rf_gain, 0, 64);
        gain_changed |= RImGui::SteppedSliderInt("IF Gain", &if_gain, 0, 127);
    }
    if (gain_changed)
        set_att();

    // Bias
    if (RImGui::Checkbox("Bias", &bias))
        set_bias();
}

void SDDCSource::set_samplerate(uint64_t samplerate)
{
    for (int i = 0; i < available_samplerates.size(); i++)
    {
        if (samplerate * 2 == available_samplerates[i])
        {
            selected_samplerate = i;
            current_samplerate = samplerate * 2;
            return;
        }
    }

    throw satdump_exception("Unsupported samplerate : " + std::to_string(samplerate * 2) + "!");
}

uint64_t SDDCSource::get_samplerate()
{
    return current_samplerate / 2;
}

std::vector<dsp::SourceDescriptor> SDDCSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    int c = sddc_get_device_count();

    for (int i = 0; i < c; i++)
    {
        sddc_t *device = sddc_open(0, resources::getResourcePath("sddc/SDDC_FX3.img").c_str());
        if (device == NULL)
            continue;

        const char *name = sddc_get_hw_model_name(device);

        results.push_back({"sddc", std::string(name) + " #" + std::to_string(i), std::to_string(i)});

        sddc_close(device);
    }

    return results;
}

#include "airspy_sdr.h"

#ifdef __ANDROID__
#include "common/dsp_sample_source/android_usb_backend.h"

#define AIRSPY_USB_VID_PID \
    {                      \
        {                  \
            0x1d50, 0x60a1 \
        }                  \
    }
#endif

int AirspySource::_rx_callback(airspy_transfer *t)
{
    std::shared_ptr<dsp::stream<complex_t>> stream = *((std::shared_ptr<dsp::stream<complex_t>> *)t->ctx);
    memcpy(stream->writeBuf, t->samples, t->sample_count * sizeof(complex_t));
    stream->swap(t->sample_count);
    return 0;
}

void AirspySource::set_gains()
{
    if (gain_type == 0)
    {
        airspy_set_sensitivity_gain(airspy_dev_obj, general_gain);
        logger->debug("Set Airspy gain (sensitive) to {:d}", general_gain);
    }
    else if (gain_type == 1)
    {
        airspy_set_linearity_gain(airspy_dev_obj, general_gain);
        logger->debug("Set Airspy gain (linear) to {:d}", general_gain);
    }
    else if (gain_type == 3)
    {
        airspy_set_lna_gain(airspy_dev_obj, manual_gains[0]);
        airspy_set_mixer_gain(airspy_dev_obj, manual_gains[1]);
        airspy_set_vga_gain(airspy_dev_obj, manual_gains[2]);
        logger->debug("Set Airspy gain (manual) to {:d}, {:d}, {:d}", manual_gains[0], manual_gains[1], manual_gains[2]);
    }
}

void AirspySource::set_bias()
{
    airspy_set_rf_bias(airspy_dev_obj, bias_enabled);
    logger->debug("Set Airspy bias to {:d}", (int)bias_enabled);
}

void AirspySource::set_agcs()
{
    airspy_set_lna_agc(airspy_dev_obj, lna_agc_enabled);
    airspy_set_mixer_agc(airspy_dev_obj, mixer_agc_enabled);
    logger->debug("Set Airspy LNA AGC to {:d}", (int)lna_agc_enabled);
    logger->debug("Set Airspy Mixer AGC to {:d}", (int)mixer_agc_enabled);
}

void AirspySource::set_settings(nlohmann::json settings)
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

    if (is_open)
    {
        set_gains();
        set_bias();
        set_agcs();
    }
}

nlohmann::json AirspySource::get_settings(nlohmann::json)
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

void AirspySource::open()
{
#ifndef __ANDROID__
    if (!is_open)
        if (airspy_open_sn(&airspy_dev_obj, d_sdr_id) != AIRSPY_SUCCESS)
            throw std::runtime_error("Could not open Airspy device!");
#else
    int vid, pid;
    std::string path;
    int fd = getDeviceFD(vid, pid, AIRSPY_USB_VID_PID, path);
    if (airspy_open2(&airspy_dev_obj, fd, path.c_str()) != AIRSPY_SUCCESS)
        throw std::runtime_error("Could not open Airspy device!");
#endif
    is_open = true;

    // Get available samplerates
    uint32_t samprate_cnt;
    uint32_t dev_samplerates[10];
    airspy_get_samplerates(airspy_dev_obj, &samprate_cnt, 0);
    airspy_get_samplerates(airspy_dev_obj, dev_samplerates, samprate_cnt);
    available_samplerates.clear();
    bool has_10msps = false;
    for (int i = samprate_cnt - 1; i >= 0; i--)
    {
        logger->trace("Airspy device has samplerate {:d} SPS", dev_samplerates[i]);
        available_samplerates.push_back(dev_samplerates[i]);
        if (dev_samplerates[i] == 10e6)
            has_10msps = true;
    }

    if (!has_10msps)
        available_samplerates.push_back(10e6);

    // Init UI stuff
    samplerate_option_str = "";
    for (uint64_t samplerate : available_samplerates)
        samplerate_option_str += std::to_string(samplerate) + '\0';
}

void AirspySource::start()
{
    DSPSampleSource::start();
    airspy_set_sample_type(airspy_dev_obj, AIRSPY_SAMPLE_FLOAT32_IQ);

    logger->debug("Set Airspy samplerate to " + std::to_string(current_samplerate));
    airspy_set_samplerate(airspy_dev_obj, current_samplerate);

    set_frequency(d_frequency);

    set_gains();
    set_bias();
    set_agcs();

    airspy_start_rx(airspy_dev_obj, &_rx_callback, &output_stream);

    is_started = true;
}

void AirspySource::stop()
{
    airspy_stop_rx(airspy_dev_obj);
    is_started = false;
}

void AirspySource::close()
{
    if (is_open)
        airspy_close(airspy_dev_obj);
}

void AirspySource::set_frequency(uint64_t frequency)
{
    if (is_open)
    {
        airspy_set_freq(airspy_dev_obj, frequency);
        logger->debug("Set Airspy frequency to {:d}", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void AirspySource::drawControlUI()
{
    if (is_started)
        style::beginDisabled();
    ImGui::Combo("Samplerate", &selected_samplerate, samplerate_option_str.c_str());
    current_samplerate = available_samplerates[selected_samplerate];
    if (is_started)
        style::endDisabled();

    // Gain settings
    bool gain_changed = false;
    gain_changed |= ImGui::RadioButton("Sensitive", &gain_type, 0);
    // ImGui::SameLine();
    gain_changed |= ImGui::RadioButton("Linear", &gain_type, 1);
    // ImGui::SameLine();
    gain_changed |= ImGui::RadioButton("Manual", &gain_type, 2);

    if (gain_type == 2)
    {
        gain_changed |= ImGui::SliderInt("LNA Gain", &manual_gains[0], 0, 15);
        gain_changed |= ImGui::SliderInt("Mixer Gain", &manual_gains[1], 0, 15);
        gain_changed |= ImGui::SliderInt("VGA Gain", &manual_gains[2], 0, 15);
    }
    else
    {
        gain_changed |= ImGui::SliderInt("Gain", &general_gain, 0, 21);
    }

    if (gain_changed)
        set_gains();

    if (ImGui::Checkbox("Bias-Tee", &bias_enabled))
        set_bias();

    if (ImGui::Checkbox("LNA AGC", &lna_agc_enabled))
        set_agcs();

    if (ImGui::Checkbox("Mixer AGC", &mixer_agc_enabled))
        set_agcs();
}

void AirspySource::set_samplerate(uint64_t samplerate)
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

uint64_t AirspySource::get_samplerate()
{
    return current_samplerate;
}

std::vector<dsp::SourceDescriptor> AirspySource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

#ifndef __ANDROID__
    uint64_t serials[100];
    int c = airspy_list_devices(serials, 100);

    for (int i = 0; i < c; i++)
    {
        std::stringstream ss;
        ss << std::hex << serials[i];
        results.push_back({"airspy", "AirSpy One " + ss.str(), serials[i]});
    }
#else
    int vid, pid;
    std::string path;
    if (getDeviceFD(vid, pid, AIRSPY_USB_VID_PID, path) != -1)
        results.push_back({"airspy", "AirSpy One USB", 0});
#endif

    return results;
}
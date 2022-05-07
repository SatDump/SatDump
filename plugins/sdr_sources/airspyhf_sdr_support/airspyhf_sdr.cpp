#include "airspyhf_sdr.h"

int AirspyHFSource::_rx_callback(airspyhf_transfer_t *t)
{
    std::shared_ptr<dsp::stream<complex_t>> stream = *((std::shared_ptr<dsp::stream<complex_t>> *)t->ctx);
    memcpy(stream->writeBuf, t->samples, t->sample_count * sizeof(complex_t));
    stream->swap(t->sample_count);
    return 0;
}

void AirspyHFSource::set_atte()
{
    airspyhf_set_hf_att(airspyhf_dev_obj, attenuation / 6.0f);
    logger->debug("Set AirspyHF HF Attentuation to {:d}", attenuation);
}

void AirspyHFSource::set_lna()
{
    airspyhf_set_hf_lna(airspyhf_dev_obj, hf_lna_enabled);
    logger->debug("Set AirspyHF HF LNA to {:d}", (int)hf_lna_enabled);
}

void AirspyHFSource::set_agcs()
{
    airspyhf_set_hf_agc(airspyhf_dev_obj, agc_mode != 0);
    airspyhf_set_hf_agc_threshold(airspyhf_dev_obj, agc_mode - 1);
    logger->debug("Set AirspyHF HF AGC Mode to {:d}", (int)agc_mode);
}

void AirspyHFSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    agc_mode = getValueOrDefault(d_settings["agc_mode"], agc_mode);
    attenuation = getValueOrDefault(d_settings["attenuation"], attenuation);
    hf_lna_enabled = getValueOrDefault(d_settings["hf_lna"], hf_lna_enabled);

    if (is_open)
    {
        set_atte();
        set_lna();
        set_agcs();
    }
}

nlohmann::json AirspyHFSource::get_settings(nlohmann::json)
{
    d_settings["agc_mode"] = agc_mode;
    d_settings["attenuation"] = attenuation;
    d_settings["hf_lna"] = hf_lna_enabled;

    return d_settings;
}

void AirspyHFSource::open()
{
    if (!is_open)
        if (airspyhf_open_sn(&airspyhf_dev_obj, d_sdr_id) != AIRSPYHF_SUCCESS)
            throw std::runtime_error("Could not open AirspyHF device!");
    is_open = true;

    // Get available samplerates
    uint32_t samprate_cnt;
    uint32_t dev_samplerates[10];
    airspyhf_get_samplerates(airspyhf_dev_obj, &samprate_cnt, 0);
    airspyhf_get_samplerates(airspyhf_dev_obj, dev_samplerates, samprate_cnt);
    available_samplerates.clear();
    for (int i = samprate_cnt - 1; i >= 0; i--)
    {
        logger->trace("AirspyHF device has samplerate {:d} SPS", dev_samplerates[i]);
        available_samplerates.push_back(dev_samplerates[i]);
    }

    // Init UI stuff
    samplerate_option_str = "";
    for (uint64_t samplerate : available_samplerates)
        samplerate_option_str += std::to_string(samplerate) + '\0';
}

void AirspyHFSource::start()
{
    DSPSampleSource::start();

    logger->debug("Set AirspyHF samplerate to " + std::to_string(current_samplerate));
    airspyhf_set_samplerate(airspyhf_dev_obj, current_samplerate);

    set_frequency(d_frequency);

    set_atte();
    set_lna();
    set_agcs();

    airspyhf_start(airspyhf_dev_obj, &_rx_callback, &output_stream);

    is_started = true;
}

void AirspyHFSource::stop()
{
    airspyhf_stop(airspyhf_dev_obj);
    is_started = false;
}

void AirspyHFSource::close()
{
    if (is_open)
        airspyhf_close(airspyhf_dev_obj);
}

void AirspyHFSource::set_frequency(uint64_t frequency)
{
    if (is_open)
    {
        airspyhf_set_freq(airspyhf_dev_obj, frequency);
        logger->debug("Set AirspyHF frequency to {:d}", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void AirspyHFSource::drawControlUI()
{
    if (is_started)
        style::beginDisabled();
    ImGui::Combo("Samplerate", &selected_samplerate, samplerate_option_str.c_str());
    current_samplerate = available_samplerates[selected_samplerate];
    if (is_started)
        style::endDisabled();

    if (ImGui::SliderInt("Attenuation", &attenuation, 0, 48))
        set_atte();

    if (ImGui::Combo("AGC Mode", &agc_mode, "OFF\0"
                                            "LOW\0"
                                            "HIGH\0"))
        set_agcs();

    if (ImGui::Checkbox("HF LNA", &hf_lna_enabled))
        set_lna();
}

void AirspyHFSource::set_samplerate(uint64_t samplerate)
{
    for (int i = 0; i < available_samplerates.size(); i++)
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

uint64_t AirspyHFSource::get_samplerate()
{
    return current_samplerate;
}

std::vector<dsp::SourceDescriptor> AirspyHFSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    uint64_t serials[100];
    int c = airspyhf_list_devices(serials, 100);

    for (int i = 0; i < c; i++)
    {
        std::stringstream ss;
        ss << std::hex << serials[i];
        results.push_back({"airspyhf", "AirSpyHF " + ss.str(), serials[i]});
    }

    return results;
}
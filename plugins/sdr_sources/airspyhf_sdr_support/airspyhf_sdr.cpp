#include "airspyhf_sdr.h"

#ifdef __ANDROID__
#include "common/dsp_source_sink/android_usb_backend.h"

const std::vector<DevVIDPID> AIRSPYHF_USB_VID_PID = {{0x03EB, 0x800C}};
#endif

int AirspyHFSource::_rx_callback(airspyhf_transfer_t *t)
{
    std::shared_ptr<dsp::stream<complex_t>> stream = *((std::shared_ptr<dsp::stream<complex_t>> *)t->ctx);
    memcpy(stream->writeBuf, (complex_t *)t->samples, t->sample_count * sizeof(complex_t));
    stream->swap(t->sample_count);
    return 0;
}

void AirspyHFSource::set_atte()
{
    if (!is_started)
        return;

    airspyhf_set_hf_att(airspyhf_dev_obj, attenuation / 6.0f);
    logger->debug("Set AirspyHF HF Attentuation to %d", attenuation);
}

void AirspyHFSource::set_lna()
{
    if (!is_started)
        return;

    airspyhf_set_hf_lna(airspyhf_dev_obj, hf_lna_enabled);
    logger->debug("Set AirspyHF HF LNA to %d", (int)hf_lna_enabled);
}

void AirspyHFSource::set_agcs()
{
    if (!is_started)
        return;

    airspyhf_set_hf_agc(airspyhf_dev_obj, agc_mode != 0);
    airspyhf_set_hf_agc_threshold(airspyhf_dev_obj, agc_mode - 1);
    logger->debug("Set AirspyHF HF AGC Mode to %d", (int)agc_mode);
}

void AirspyHFSource::open_sdr()
{
#ifndef __ANDROID__
    if (airspyhf_open_sn(&airspyhf_dev_obj, d_sdr_id) != AIRSPYHF_SUCCESS)
        throw satdump_exception("Could not open AirspyHF device!");
#else
    int vid, pid;
    std::string path;
    int fd = getDeviceFD(vid, pid, AIRSPYHF_USB_VID_PID, path);
    if (airspyhf_open_fd(&airspyhf_dev_obj, fd) != AIRSPYHF_SUCCESS)
        throw satdump_exception("Could not open AirspyHF device!");
#endif
}

void AirspyHFSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    agc_mode = getValueOrDefault(d_settings["agc_mode"], agc_mode);
    attenuation = getValueOrDefault(d_settings["attenuation"], attenuation);
    hf_lna_enabled = getValueOrDefault(d_settings["hf_lna"], hf_lna_enabled);

    if (is_started)
    {
        set_atte();
        set_lna();
        set_agcs();
    }
}

nlohmann::json AirspyHFSource::get_settings()
{
    d_settings["agc_mode"] = agc_mode;
    d_settings["attenuation"] = attenuation;
    d_settings["hf_lna"] = hf_lna_enabled;

    return d_settings;
}

void AirspyHFSource::open()
{
    open_sdr();
    is_open = true;

    // Get available samplerates
    uint32_t samprate_cnt;
    uint32_t dev_samplerates[10];
    airspyhf_get_samplerates(airspyhf_dev_obj, &samprate_cnt, 0);
    airspyhf_get_samplerates(airspyhf_dev_obj, dev_samplerates, samprate_cnt);
    std::vector<double> available_samplerates;
    for (int i = samprate_cnt - 1; i >= 0; i--)
    {
        logger->trace("AirspyHF device has samplerate %d SPS", dev_samplerates[i]);
        available_samplerates.push_back(dev_samplerates[i]);
    }

    samplerate_widget.set_list(available_samplerates, true);
    airspyhf_close(airspyhf_dev_obj);
}

void AirspyHFSource::start()
{
    DSPSampleSource::start();
    open_sdr();

    uint64_t current_samplerate = samplerate_widget.get_value();

    logger->debug("Set AirspyHF samplerate to " + std::to_string(current_samplerate));
    airspyhf_set_samplerate(airspyhf_dev_obj, current_samplerate);

    is_started = true;

    set_frequency(d_frequency);

    set_atte();
    set_lna();
    set_agcs();

    airspyhf_start(airspyhf_dev_obj, &_rx_callback, &output_stream);
}

void AirspyHFSource::stop()
{
    if (is_started)
    {
        airspyhf_stop(airspyhf_dev_obj);
        airspyhf_close(airspyhf_dev_obj);
    }
    is_started = false;
}

void AirspyHFSource::close()
{
    is_open = false;
}

void AirspyHFSource::set_frequency(uint64_t frequency)
{
    if (is_started)
    {
        airspyhf_set_freq(airspyhf_dev_obj, frequency);
        logger->debug("Set AirspyHF frequency to %d", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void AirspyHFSource::drawControlUI()
{
    if (is_started)
        RImGui::beginDisabled();

    samplerate_widget.render();

    if (is_started)
        RImGui::endDisabled();

    if (RImGui::SteppedSliderInt("Attenuation", &attenuation, 0, 48))
        set_atte();

    if (RImGui::Combo("AGC Mode", &agc_mode, "OFF\0"
                                            "LOW\0"
                                            "HIGH\0"))
        set_agcs();

    if (RImGui::Checkbox("HF LNA", &hf_lna_enabled))
        set_lna();
}

void AirspyHFSource::set_samplerate(uint64_t samplerate)
{
    if (!samplerate_widget.set_value(samplerate, 3.2e6))
        throw satdump_exception("Unspported samplerate : " + std::to_string(samplerate) + "!");
}

uint64_t AirspyHFSource::get_samplerate()
{
    return samplerate_widget.get_value();
}

std::vector<dsp::SourceDescriptor> AirspyHFSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

#ifndef __ANDROID__
    uint64_t serials[100];
    int c = airspyhf_list_devices(serials, 100);

    for (int i = 0; i < c; i++)
    {
        std::stringstream ss;
        ss << std::hex << serials[i];
        results.push_back({"airspyhf", "AirSpyHF " + ss.str(), serials[i]});
    }
#else
    int vid, pid;
    std::string path;
    if (getDeviceFD(vid, pid, AIRSPYHF_USB_VID_PID, path) != -1)
        results.push_back({"airspyhf", "AirSpyHF USB", 0});
#endif

    return results;
}
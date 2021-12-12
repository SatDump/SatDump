#include "airspyhf.h"

#ifndef DISABLE_SDR_AIRSPYHF
#include <sstream>
#include "imgui/imgui.h"
#include "logger.h"

int SDRAirspyHF::_rx_callback(airspyhf_transfer_t *t)
{
    std::shared_ptr<dsp::stream<complex_t>> stream = *((std::shared_ptr<dsp::stream<complex_t>> *)t->ctx);
    std::memcpy(stream->writeBuf, (complex_t *)t->samples, t->sample_count * sizeof(complex_t));
    stream->swap(t->sample_count);
    return 0;
};

SDRAirspyHF::SDRAirspyHF(std::map<std::string, std::string> parameters, uint64_t id) : SDRDevice(parameters, id)
{
    READ_PARAMETER_IF_EXISTS_FLOAT(agc_mode, "agc_mode");
    READ_PARAMETER_IF_EXISTS_FLOAT(attenuation, "attenuation");
    READ_PARAMETER_IF_EXISTS_FLOAT(hf_lna, "hf_lna");

    if (airspyhf_open_sn(&dev, id) != AIRSPYHF_SUCCESS)
    {
        logger->critical("Could not open AirspyHF device!");
        return;
    }
    logger->info("Opened AirspyHF device!");
    std::fill(frequency, &frequency[100], 0);
}

#ifdef __ANDROID__
SDRAirspyHF::SDRAirspyHF(std::map<std::string, std::string> parameters, int fileDescriptor, std::string devicePath) : SDRDevice(parameters, 0)
{
    READ_PARAMETER_IF_EXISTS_FLOAT(agc_mode, "agc_mode");
    READ_PARAMETER_IF_EXISTS_FLOAT(attenuation, "attenuation");
    READ_PARAMETER_IF_EXISTS_FLOAT(hf_lna, "hf_lna");

    if (airspyhf_open2(&dev, fileDescriptor, devicePath.c_str()) != AIRSPYHF_SUCCESS)
    {
        logger->critical("Could not open AirspyHF device!");
        return;
    }
    logger->info("Opened AirspyHF device!");
    std::fill(frequency, &frequency[100], 0);
}
#endif

std::map<std::string, std::string> SDRAirspyHF::getParameters()
{
    d_parameters["agc_mode"] = std::to_string(agc_mode);
    d_parameters["attenuation"] = std::to_string(attenuation);
    d_parameters["hf_lna"] = std::to_string(hf_lna);

    return d_parameters;
}

void SDRAirspyHF::start()
{
    //airspy_set_sample_type(dev, AIRSPYHF_SAMPLE_FLOAT32_IQ);

    logger->info("Samplerate " + std::to_string(d_samplerate));
    airspyhf_set_samplerate(dev, d_samplerate);

    logger->info("Frequency " + std::to_string(d_frequency));
    airspyhf_set_freq(dev, d_frequency);

    airspyhf_set_hf_agc(dev, agc_mode != 0);
    airspyhf_set_hf_agc_threshold(dev, agc_mode - 1);

    airspyhf_set_hf_att(dev, attenuation / 6.0f);
    airspyhf_set_hf_lna(dev, hf_lna);

    airspyhf_start(dev, &_rx_callback, &output_stream);
}

void SDRAirspyHF::stop()
{
    airspyhf_stop(dev);
}

SDRAirspyHF::~SDRAirspyHF()
{
    airspyhf_close(dev);
}

void SDRAirspyHF::drawUI()
{
    ImGui::Begin("AirspyHF Control", NULL);

    //ImGui::SetNextItemWidth(100);
    ImGui::InputText("MHz", frequency, 100);

    ImGui::SameLine();

    if (ImGui::Button("Set"))
    {
        d_frequency = std::stof(frequency) * 1e6;
        airspyhf_set_freq(dev, d_frequency);
    }

    //ImGui::SetNextItemWidth(200);
    if (ImGui::SliderInt("Attenuation", &attenuation, 0, 48))
    {
        airspyhf_set_hf_att(dev, attenuation / 6.0f);
    }

    if (ImGui::Combo("LNA Mode", &agc_mode, "OFF\0"
                                            "LOW\0"
                                            "HIGH\0"))
    {
        airspyhf_set_hf_agc(dev, agc_mode != 0);
        airspyhf_set_hf_agc_threshold(dev, agc_mode - 1);
    }

    if (ImGui::Checkbox("HF LNA", &hf_lna))
    {
        airspyhf_set_hf_lna(dev, hf_lna);
    }
    ImGui::End();
}

void SDRAirspyHF::setFrequency(float frequency)
{
    d_frequency = frequency;
    std::memcpy(this->frequency, std::to_string((float)d_frequency / 1e6).c_str(), std::to_string((float)d_frequency / 1e6).length());
    airspyhf_set_freq(dev, d_frequency);
}

void SDRAirspyHF::init()
{
}

std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> SDRAirspyHF::getDevices()
{
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> results;

    uint64_t serials[100];
    int c = airspyhf_list_devices(serials, 100);

    for (int i = 0; i < c; i++)
    {
        std::stringstream ss;
        ss << std::hex << serials[i];
        results.push_back({"AirSpyHF " + ss.str(), AIRSPYHF, serials[i]});
    }

#ifdef __ANDROID__
    results.push_back({"AirspyHF Device", AIRSPYHF, 0});
#endif

    return results;
}

std::string SDRAirspyHF::getID()
{
    return "airspyhf";
}
#endif
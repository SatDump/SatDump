#include "airspy.h"

#ifndef DISABLE_SDR_AIRSPY
#include <sstream>
#include "imgui/imgui.h"
#include "logger.h"

int SDRAirspy::_rx_callback(airspy_transfer *t)
{
    std::shared_ptr<dsp::stream<complex_t>> stream = *((std::shared_ptr<dsp::stream<complex_t>> *)t->ctx);
    std::memcpy(stream->writeBuf, t->samples, t->sample_count * sizeof(complex_t));
    stream->swap(t->sample_count);
    return 0;
};

SDRAirspy::SDRAirspy(std::map<std::string, std::string> parameters, uint64_t id) : SDRDevice(parameters, id)
{
    READ_PARAMETER_IF_EXISTS_FLOAT(gain, "gain");
    READ_PARAMETER_IF_EXISTS_FLOAT(bias, "bias");

    if (airspy_open_sn(&dev, id) != AIRSPY_SUCCESS)
    {
        logger->critical("Could not open Airspy device!");
        return;
    }
    logger->info("Opened Airspy device!");
    std::fill(frequency, &frequency[100], 0);
}

#ifdef __ANDROID__
SDRAirspy::SDRAirspy(std::map<std::string, std::string> parameters, int fileDescriptor, std::string devicePath) : SDRDevice(parameters, 0)
{
    READ_PARAMETER_IF_EXISTS_FLOAT(gain, "gain");
    READ_PARAMETER_IF_EXISTS_FLOAT(bias, "bias");

    if (airspy_open2(&dev, fileDescriptor, devicePath.c_str()) != AIRSPY_SUCCESS)
    {
        logger->critical("Could not open Airspy device!");
        return;
    }
    logger->info("Opened Airspy device!");
    std::fill(frequency, &frequency[100], 0);
}
#endif

std::map<std::string, std::string> SDRAirspy::getParameters()
{
    d_parameters["gain"] = std::to_string(gain);
    d_parameters["bias"] = std::to_string(bias);

    return d_parameters;
}

void SDRAirspy::start()
{
    airspy_set_sample_type(dev, AIRSPY_SAMPLE_FLOAT32_IQ);

    logger->info("Samplerate " + std::to_string(d_samplerate));
    airspy_set_samplerate(dev, d_samplerate);

    logger->info("Frequency " + std::to_string(d_frequency));
    airspy_set_freq(dev, d_frequency);

    airspy_set_rf_bias(dev, bias);
    airspy_set_linearity_gain(dev, gain);
    airspy_start_rx(dev, &_rx_callback, &output_stream);
}

void SDRAirspy::stop()
{
    airspy_stop_rx(dev);
}

SDRAirspy::~SDRAirspy()
{
    airspy_close(dev);
}

void SDRAirspy::drawUI()
{
    ImGui::Begin("Airspy Control", NULL);

    //ImGui::SetNextItemWidth(100);
    ImGui::InputText("MHz", frequency, 100);

    ImGui::SameLine();

    if (ImGui::Button("Set"))
    {
        d_frequency = std::stof(frequency) * 1e6;
        airspy_set_freq(dev, d_frequency);
    }

    //ImGui::SetNextItemWidth(200);
    if (ImGui::SliderInt("Gain", &gain, 0, 22))
    {
        airspy_set_linearity_gain(dev, gain);
    }

    if (ImGui::Checkbox("Bias", &bias))
    {
        airspy_set_rf_bias(dev, bias);
    }
    ImGui::End();
}

void SDRAirspy::setFrequency(float frequency)
{
    d_frequency = frequency;
    std::memcpy(this->frequency, std::to_string((float)d_frequency / 1e6).c_str(), std::to_string((float)d_frequency / 1e6).length());
    airspy_set_freq(dev, d_frequency);
}

void SDRAirspy::init()
{
}

std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> SDRAirspy::getDevices()
{
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> results;

    uint64_t serials[100];
    int c = airspy_list_devices(serials, 100);

    for (int i = 0; i < c; i++)
    {
        std::stringstream ss;
        ss << std::hex << serials[i];
        results.push_back({"AirSpy One " + ss.str(), AIRSPY, serials[i]});
    }

#ifdef __ANDROID__
    results.push_back({"Airspy Device", AIRSPY, 0});
#endif

    return results;
}

std::string SDRAirspy::getID()
{
    return "airspy";
}
#endif
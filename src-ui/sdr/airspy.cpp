#include "airspy.h"
#include <sstream>
#include "live.h"
#include "imgui/imgui.h"
#include "logger.h"

#ifdef BUILD_LIVE

int SDRAirspy::_rx_callback(airspy_transfer *t)
{
    std::shared_ptr<dsp::stream<std::complex<float>>> stream = *((std::shared_ptr<dsp::stream<std::complex<float>>> *)t->ctx);
    std::memcpy(stream->writeBuf, t->samples, t->sample_count * sizeof(std::complex<float>));
    stream->swap(t->sample_count);
    return 0;
};

SDRAirspy::SDRAirspy(uint64_t id) : SDRDevice(id)
{
    if (airspy_open_sn(&dev, id) != AIRSPY_SUCCESS)
    {
        logger->critical("Could not open Airspy device!");
        return;
    }
    logger->info("Opened Airspy device!");
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

    ImGui::SetNextItemWidth(100);
    ImGui::InputText("MHz", frequency, 100);

    ImGui::SameLine();

    if (ImGui::Button("Set"))
    {
        d_frequency = std::stof(frequency) * 1e6;
        airspy_set_freq(dev, d_frequency);
    }

    ImGui::SetNextItemWidth(200);
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

void SDRAirspy::setFrequency(int frequency)
{
    d_frequency = frequency;
    std::memcpy(this->frequency, std::to_string((float)d_frequency / 1e6).c_str(), std::to_string((float)d_frequency / 1e6).length());
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
    return results;
}

#endif
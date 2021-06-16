#include "hackrf.h"

#ifndef DISABLE_SDR_HACKRF
#include <sstream>
#include "imgui/imgui.h"
#include "logger.h"

int SDRHackRF::_rx_callback(hackrf_transfer *t)
{
    std::shared_ptr<dsp::stream<std::complex<float>>> stream = *((std::shared_ptr<dsp::stream<std::complex<float>>> *)t->rx_ctx);
    // Convert to CF-32
    for (int i = 0; i < t->buffer_length / 2; i++)
        stream->writeBuf[i] = std::complex<float>(((int8_t *)t->buffer)[i * 2 + 0] / 128.0f, ((int8_t *)t->buffer)[i * 2 + 1] / 128.0f);
    stream->swap(t->buffer_length / 2);
    return 0;
};

SDRHackRF::SDRHackRF(std::map<std::string, std::string> parameters, uint64_t id) : SDRDevice(parameters, id)
{
    READ_PARAMETER_IF_EXISTS_FLOAT(lna_gain, "lna_gain");
    READ_PARAMETER_IF_EXISTS_FLOAT(vga_gain, "vga_gain");
    READ_PARAMETER_IF_EXISTS_FLOAT(amp, "amp");
    READ_PARAMETER_IF_EXISTS_FLOAT(bias, "bias");

    std::stringstream ss;
    ss << std::hex << id;
    if (!hackrf_open_by_serial(ss.str().c_str(), &dev))
    {
        logger->critical("Could not open HackRF device!");
        return;
    }
    hackrf_reset(dev);
    logger->info("Opened HackRF device!");
}

std::map<std::string, std::string> SDRHackRF::getParameters()
{
    d_parameters["lna_gain"] = std::to_string(lna_gain);
    d_parameters["vga_gain"] = std::to_string(vga_gain);
    d_parameters["amp"] = std::to_string(amp);
    d_parameters["bias"] = std::to_string(bias);

    return d_parameters;
}

void SDRHackRF::start()
{
    logger->info("Samplerate " + std::to_string(d_samplerate));
    hackrf_set_sample_rate(dev, d_samplerate);
    hackrf_set_baseband_filter_bandwidth(dev, d_samplerate);

    logger->info("Frequency " + std::to_string(d_frequency));
    setFrequency(d_frequency);

    hackrf_set_amp_enable(dev, amp);
    hackrf_set_lna_gain(dev, lna_gain);
    hackrf_set_vga_gain(dev, vga_gain);

    hackrf_start_rx(dev, _rx_callback, &output_stream);
}

void SDRHackRF::stop()
{
    hackrf_stop_rx(dev);
}

SDRHackRF::~SDRHackRF()
{
    hackrf_close(dev);
}

void SDRHackRF::drawUI()
{
    ImGui::Begin("HackRF Control", NULL);

    ImGui::SetNextItemWidth(100);
    ImGui::InputText("MHz", frequency, 100);

    ImGui::SameLine();

    if (ImGui::Button("Set"))
    {
        d_frequency = std::stof(frequency) * 1e6;
        setFrequency(d_frequency);
    }

    ImGui::SetNextItemWidth(200);
    if (ImGui::SliderInt("LNA Gain", &lna_gain, 0, 49))
    {
        hackrf_set_lna_gain(dev, lna_gain);
    }

    ImGui::SetNextItemWidth(200);
    if (ImGui::SliderInt("VGA Gain", &vga_gain, 0, 49))
    {
        hackrf_set_vga_gain(dev, vga_gain);
    }

    if (ImGui::Checkbox("AMP", &amp))
    {
        hackrf_set_amp_enable(dev, amp);
    }

    ImGui::SameLine();

    if (ImGui::Checkbox("Bias", &bias))
    {
        hackrf_set_antenna_enable(dev, bias);
    }

    ImGui::End();
}

void SDRHackRF::setFrequency(float frequency)
{
    d_frequency = frequency;
    std::memcpy(this->frequency, std::to_string((float)d_frequency / 1e6).c_str(), std::to_string((float)d_frequency / 1e6).length());

    if (hackrf_set_freq(dev, d_frequency) != 0)
    {
        logger->error("Could not set SDR frequency!");
    }
}

void SDRHackRF::init()
{
    hackrf_init();
}

std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> SDRHackRF::getDevices()
{
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> results;

    hackrf_device_list_t *devlist = hackrf_device_list();

    for (int i = 0; i < devlist->devicecount; i++)
    {
        std::stringstream ss, ss2;
        uint64_t id = 0;
        ss << devlist->serial_numbers[i];
        ss >> std::hex >> id;
        ss << devlist->serial_numbers[i];
        results.push_back({"HackRF One " + ss.str(), HACKRF, id});
    }
    return results;
}

std::string SDRHackRF::getID()
{
    return "hackrf";
}
#endif

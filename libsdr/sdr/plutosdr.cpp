#include "plutosdr.h"

#ifndef DISABLE_SDR_PLUTOSDR
#include <sstream>
#include "imgui/imgui.h"
#include "logger.h"
#include <iio.h>
#include <ad9361.h>

// code practically copypasted from https://github.com/AlexandreRouma/SDRPlusPlus/blob/master/source_modules/plutosdr_source/src/main.cpp

void SDRPlutoSDR::worker()
{
    int blockSize = d_samplerate / 200.0f;

    struct iio_channel *rx0_i, *rx0_q;
    struct iio_buffer *rxbuf;

    rx0_i = iio_device_find_channel(dev, "voltage0", 0);
    rx0_q = iio_device_find_channel(dev, "voltage1", 0);

    iio_channel_enable(rx0_i);
    iio_channel_enable(rx0_q);

    rxbuf = iio_device_create_buffer(dev, blockSize, false);
    if (!rxbuf)
    {
        logger->error("Could not create RX buffer");
        return;
    }

    while (running)
    {
        iio_buffer_refill(rxbuf);

        int16_t *buf = (int16_t*)iio_buffer_first(rxbuf, rx0_i);

        volk_16i_s32f_convert_32f((float*)output_stream->writeBuf, buf, 32768.0f, blockSize * 2);

        if (!output_stream->swap(blockSize))
        {
            break;
        };
    }
    iio_buffer_destroy(rxbuf);
}

SDRPlutoSDR::SDRPlutoSDR(std::map<std::string, std::string> parameters, uint64_t id) : SDRDevice(parameters, id)
{
    strcpy(&ip[3], parameters["ip"].c_str());
    std::fill(frequency, &frequency[100], 0);
}

std::map<std::string, std::string> SDRPlutoSDR::getParameters()
{
    d_parameters["gain"] = std::to_string(gain);

    return d_parameters;
}

void SDRPlutoSDR::start()
{
    logger->info("Samplerate " + std::to_string(d_samplerate));
    logger->info("Frequency " + std::to_string(d_frequency));

    ctx = iio_create_context_from_uri(ip);
    if (ctx == NULL)
    {
        logger->error("Could not open pluto");
        return;
    }
    phy = iio_context_find_device(ctx, "ad9361-phy");
    if (phy == NULL)
    {
        logger->error("Could not connect to pluto phy");
        iio_context_destroy(ctx);
        return;
    }
    dev = iio_context_find_device(ctx, "cf-ad9361-lpc");
    if (dev == NULL)
    {
        logger->error("Could not connect to pluto dev");
        iio_context_destroy(ctx);
        return;
    }

    iio_channel_attr_write_bool(iio_device_find_channel(phy, "altvoltage1", true), "powerdown", true);
    iio_channel_attr_write_bool(iio_device_find_channel(phy, "altvoltage0", true), "powerdown", false);
    iio_channel_attr_write(iio_device_find_channel(phy, "voltage0", false), "rf_port_select", "A_BALANCED");
    iio_channel_attr_write_longlong(iio_device_find_channel(phy, "voltage0", false), "sampling_frequency", round(d_samplerate)); // SR
    iio_channel_attr_write(iio_device_find_channel(phy, "voltage0", false), "gain_control_mode", "manual");                      // gain mode
    iio_channel_attr_write_longlong(iio_device_find_channel(phy, "voltage0", false), "hardwaregain", round(gain));               // gain
    iio_channel_attr_write_longlong(iio_device_find_channel(phy, "altvoltage0", true), "frequency", round(d_frequency));         //
    ad9361_set_bb_rate(phy, round(d_samplerate));

    running = true;

    workThread = std::thread(&SDRPlutoSDR::worker, this);
}

void SDRPlutoSDR::stop()
{
    running = false;
    if (workThread.joinable())
        workThread.join();
    if (ctx != NULL)
    {
        iio_context_destroy(ctx);
        ctx = NULL;
    }
}

SDRPlutoSDR::~SDRPlutoSDR()
{
    stop();
}

void SDRPlutoSDR::drawUI()
{
    ImGui::Begin("PlutoSDR Control", NULL);
    ImGui::InputText("MHz", frequency, 100);

    ImGui::SameLine();

    if (ImGui::Button("Set"))
    {
        d_frequency = std::stof(frequency) * 1e6;
        setFrequency(d_frequency);
    }

    if (ImGui::SliderFloat("Gain", &gain, 0, 76))
    {
        setGain(gain);
    }

    ImGui::End();
}

char SDRPlutoSDR::pluto_ip[100] = "192.168.2.1";
std::map<std::string, std::string> SDRPlutoSDR::drawParamsUI()
{
    ImGui::Text("IP");
    ImGui::SameLine();
    ImGui::InputText("##plutoip", pluto_ip, 100);

    return {{"ip", std::string(pluto_ip)}};
}

void SDRPlutoSDR::setFrequency(float frequency)
{
    d_frequency = frequency;
    std::memcpy(this->frequency, std::to_string((float)d_frequency / 1e6).c_str(), std::to_string((float)d_frequency / 1e6).length());
    if (running)
    {
        iio_channel_attr_write_longlong(iio_device_find_channel(phy, "altvoltage0", true), "frequency", round(d_frequency));
    }
}

void SDRPlutoSDR::setGain(float _gain)
{
    gain = _gain;
    if (running)
    {
        iio_channel_attr_write_longlong(iio_device_find_channel(phy, "voltage0", false), "hardwaregain", round(gain));
    }
}

std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> SDRPlutoSDR::getDevices()
{
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> results;

    results.push_back({std::string("PlutoSDR"), PLUTOSDR, 0});

    return results;
}

std::string SDRPlutoSDR::getID()
{
    return "plutosdr";
}

#endif
#include "pluto.h"
#include <sstream>
#include "imgui/imgui.h"
#include "logger.h"

#ifndef DISABLE_SDR_PLUTO

#define IIO_BUFFER_SIZE 32768

SDRPluto::SDRPluto(std::map<std::string, std::string> parameters, uint64_t id) : SDRDevice(parameters, id)
{
	ctx = iio_create_context_from_uri(uri);
    
    if(!ctx) {
        logger->error("Unable to open IIO context!");
    }

	phy = iio_context_find_device(ctx, "ad9361-phy");

    logger->info("Opened Pluto SDR.");

    std::fill(frequency, &frequency[100], 0);
}

void SDRPluto::start()
{
    logger->info("Frequency " + std::to_string(d_frequency));
	iio_channel_attr_write_longlong(
		iio_device_find_channel(phy, "altvoltage0", true),
		"frequency",
		d_frequency);
 
    logger->info("Samplerate " + std::to_string(d_samplerate));
	iio_channel_attr_write_longlong(
		iio_device_find_channel(phy, "voltage0", false),
		"sampling_frequency",
		d_samplerate);

	dev = iio_context_find_device(ctx, "cf-ad9361-lpc");
 
	rx0_i = iio_device_find_channel(dev, "voltage0", 0);
	rx0_q = iio_device_find_channel(dev, "voltage1", 0);
 
	iio_channel_enable(rx0_i);
	iio_channel_enable(rx0_q);
 
    iio_channel_attr_write(
        iio_device_find_channel(phy, "voltage0", false), 
        "gain_control_mode", 
        "manual");

    iio_channel_attr_write_longlong(
        iio_device_find_channel(phy, "voltage0", false), 
        "hardwaregain", 
        gain);

	rxbuf = iio_device_create_buffer(dev, IIO_BUFFER_SIZE, false);

	if (!rxbuf) {
	    logger->error("Can't create IIO buffer!");
	}

    should_run = true;
    workThread = std::thread(&SDRPluto::runThread, this);
}

std::map<std::string, std::string> SDRPluto::getParameters()
{
    d_parameters["gain"] = std::to_string(gain);

    return d_parameters;
}

void SDRPluto::runThread()
{
    while (should_run)
    {
		void *p_dat, *p_end, *t_dat;
		ptrdiff_t p_inc;
        int i = 0;

		int s = iio_buffer_refill(rxbuf);
 
		p_inc = iio_buffer_step(rxbuf);
		p_end = iio_buffer_end(rxbuf);
 
		for (p_dat = iio_buffer_first(rxbuf, rx0_i); p_dat < p_end; p_dat += p_inc, t_dat += p_inc) {
			output_stream->writeBuf[i++]  = std::complex<float>(((int16_t*)p_dat)[0] / 2048.0f, ((int16_t*)p_dat)[1] / 2048.0f);
		}

        output_stream->swap(i);
    }
}

void SDRPluto::stop()
{
    should_run = false;
    logger->info("Waiting for the thread...");
    output_stream->stopWriter();
    if (workThread.joinable())
        workThread.join();
    logger->info("Thread stopped");
    iio_channel_disable(rx0_i);
    iio_channel_disable(rx0_q);
    logger->info("Stopped Pluto");
}

SDRPluto::~SDRPluto()
{
    iio_buffer_destroy(rxbuf);
	iio_context_destroy(ctx);
}

void SDRPluto::drawUI()
{
    ImGui::Begin("Pluto Control", NULL);

    ImGui::SetNextItemWidth(100);
    ImGui::InputText("MHz", frequency, 100);

    ImGui::SameLine();

    if (ImGui::Button("Set"))
    {
        d_frequency = std::stof(frequency) * 1e6;
	    iio_channel_attr_write_longlong(
            iio_device_find_channel(phy, "altvoltage0", true),
            "frequency",
            d_frequency);
    }

    ImGui::SetNextItemWidth(200);
    if (ImGui::SliderInt("Gain", &gain, 0, 73))
    {
        iio_channel_attr_write_longlong(
            iio_device_find_channel(phy, "voltage0", false), 
            "hardwaregain", 
            gain);
    }

    ImGui::End();
}

void SDRPluto::setFrequency(float frequency)
{
    d_frequency = frequency;
    std::memcpy(this->frequency, std::to_string((float)d_frequency / 1e6).c_str(), std::to_string((float)d_frequency / 1e6).length());
}

void SDRPluto::init()
{
}

std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> SDRPluto::getDevices()
{
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> results;

    results.push_back({"Pluto", PLUTO, 0});

    return results;
}


char SDRPluto::uri[100] = "ip:pluto.local";

std::map<std::string, std::string> SDRPluto::drawParamsUI()
{
    ImGui::Text("URI");
    ImGui::SameLine();
    ImGui::InputText("##plutouri", uri, 100);

    return {{"uri", std::string(uri)}};
}


std::string SDRPluto::getID()
{
    return "pluto";
}

#endif

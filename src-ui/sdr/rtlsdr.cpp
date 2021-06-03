#include "rtlsdr.h"
#include <sstream>
#include "live.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <volk/volk.h>

#ifdef BUILD_LIVE

void SDRRtlSdr::_rx_callback(unsigned char *buf, uint32_t len, void *ctx)
{
    std::shared_ptr<dsp::stream<std::complex<float>>> stream = *((std::shared_ptr<dsp::stream<std::complex<float>>> *)ctx);
    // Convert to CF-32
    for (int i = 0; i < (int)len / 2; i++)
        stream->writeBuf[i] = std::complex<float>((buf[i * 2 + 0] - 127) / 128.0f, (buf[i * 2 + 1] - 127) / 128.0f);
    stream->swap(len / 2);
};

void SDRRtlSdr::runThread()
{
    while (should_run)
    {
        dev_mutex.lock();
        rtlsdr_read_async(dev, _rx_callback, &output_stream, 0, 16384);
        dev_mutex.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

SDRRtlSdr::SDRRtlSdr(std::map<std::string, std::string> parameters, uint64_t id) : SDRDevice(parameters, id)
{
    if (rtlsdr_open(&dev, id) > 0)
    {
        logger->critical("Could not open RTL-SDR device!");
        return;
    }
    logger->info("Opened RTL-SDR device!");
}

void SDRRtlSdr::start()
{
    logger->info("Samplerate " + std::to_string(d_samplerate));
    rtlsdr_set_sample_rate(dev, d_samplerate);

    logger->info("Frequency " + std::to_string(d_frequency));
    rtlsdr_set_center_freq(dev, d_frequency);

    rtlsdr_set_tuner_gain_mode(dev, 1);
    rtlsdr_set_tuner_gain(dev, gain);
    rtlsdr_reset_buffer(dev);
    should_run = true;
    workThread = std::thread(&SDRRtlSdr::runThread, this);
}

void SDRRtlSdr::stop()
{
    should_run = false;
    rtlsdr_cancel_async(dev);
    if (workThread.joinable())
        workThread.join();
}

SDRRtlSdr::~SDRRtlSdr()
{
    rtlsdr_close(dev);
}

void SDRRtlSdr::drawUI()
{
    ImGui::Begin("RTL-SDR Control", NULL);

    ImGui::SetNextItemWidth(100);
    ImGui::InputText("MHz", frequency, 100);

    ImGui::SameLine();

    if (ImGui::Button("Set"))
    {
        d_frequency = std::stof(frequency) * 1e6;
        setFrequency(d_frequency);
    }

    ImGui::SetNextItemWidth(200);
    if (ImGui::SliderInt("Gain", &gain, 0, 49))
    {
        setGain(gain);
    }

    if (ImGui::Checkbox("AGC", &agc))
    {
        setGainMode(agc);
    }

#ifdef HAS_RTLSDR_SET_BIAS_TEE
    ImGui::SameLine();

    if (ImGui::Checkbox("Bias", &bias))
    {
        setBias(bias);
    }
#endif

    ImGui::End();
}

void SDRRtlSdr::setFrequency(int frequency)
{
    d_frequency = frequency;
    std::memcpy(this->frequency, std::to_string((float)d_frequency / 1e6).c_str(), std::to_string((float)d_frequency / 1e6).length());

    if (should_run)
        rtlsdr_cancel_async(dev);
    dev_mutex.lock();
    while (rtlsdr_set_center_freq(dev, d_frequency) != 0)
    {
        logger->error("Could not set SDR frequency!");
    }
    dev_mutex.unlock();
}

void SDRRtlSdr::setGainMode(bool gainmode)
{
    if (should_run)
        rtlsdr_cancel_async(dev);
    dev_mutex.lock();
    while (rtlsdr_set_tuner_gain_mode(dev, !gainmode) != 0)
    {
        logger->error("Could not set SDR gain mode!");
    }
    dev_mutex.unlock();
}

void SDRRtlSdr::setGain(int gain)
{

    if (should_run)
        rtlsdr_cancel_async(dev);
    dev_mutex.lock();
    while (rtlsdr_set_tuner_gain(dev, gain * 10) != 0)
    {
        logger->error("Could not set SDR gain!");
    }
    dev_mutex.unlock();
}

void SDRRtlSdr::setBias(bool bias)
{
#ifdef HAS_RTLSDR_SET_BIAS_TEE
    if (should_run)
        rtlsdr_cancel_async(dev);
    dev_mutex.lock();
    while (rtlsdr_set_bias_tee(dev, bias) != 0)
    {
        logger->error("Could not set SDR bias tee!");
    }
    dev_mutex.unlock();
#endif
}

void SDRRtlSdr::init()
{
}

std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> SDRRtlSdr::getDevices()
{
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> results;

    int c = rtlsdr_get_device_count();

    for (int i = 0; i < c; i++)
    {
        const char *name = rtlsdr_get_device_name(i);
        results.push_back({std::string(name) + " #" + std::to_string(i), RTLSDR, i});
    }
    return results;
}

#endif
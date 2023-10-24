#include "rtlsdr_sdr.h"

#ifdef __ANDROID__
#include "common/dsp_source_sink/android_usb_backend.h"

const std::vector<DevVIDPID> RTLSDR_USB_VID_PID = {{0x0bda, 0x2832},
                                                   {0x0bda, 0x2838},
                                                   {0x0413, 0x6680},
                                                   {0x0413, 0x6f0f},
                                                   {0x0458, 0x707f},
                                                   {0x0ccd, 0x00a9},
                                                   {0x0ccd, 0x00b3},
                                                   {0x0ccd, 0x00b4},
                                                   {0x0ccd, 0x00b5},
                                                   {0x0ccd, 0x00b7},
                                                   {0x0ccd, 0x00b8},
                                                   {0x0ccd, 0x00b9},
                                                   {0x0ccd, 0x00c0},
                                                   {0x0ccd, 0x00c6},
                                                   {0x0ccd, 0x00d3},
                                                   {0x0ccd, 0x00d7},
                                                   {0x0ccd, 0x00e0},
                                                   {0x1554, 0x5020},
                                                   {0x15f4, 0x0131},
                                                   {0x15f4, 0x0133},
                                                   {0x185b, 0x0620},
                                                   {0x185b, 0x0650},
                                                   {0x185b, 0x0680},
                                                   {0x1b80, 0xd393},
                                                   {0x1b80, 0xd394},
                                                   {0x1b80, 0xd395},
                                                   {0x1b80, 0xd397},
                                                   {0x1b80, 0xd398},
                                                   {0x1b80, 0xd39d},
                                                   {0x1b80, 0xd3a4},
                                                   {0x1b80, 0xd3a8},
                                                   {0x1b80, 0xd3af},
                                                   {0x1b80, 0xd3b0},
                                                   {0x1d19, 0x1101},
                                                   {0x1d19, 0x1102},
                                                   {0x1d19, 0x1103},
                                                   {0x1d19, 0x1104},
                                                   {0x1f4d, 0xa803},
                                                   {0x1f4d, 0xb803},
                                                   {0x1f4d, 0xc803},
                                                   {0x1f4d, 0xd286},
                                                   {0x1f4d, 0xd803}};
#endif

void RtlSdrSource::_rx_callback(unsigned char *buf, uint32_t len, void *ctx)
{
    std::shared_ptr<dsp::stream<complex_t>> stream = *((std::shared_ptr<dsp::stream<complex_t>> *)ctx);
    for (int i = 0; i < (int)len / 2; i++)
        stream->writeBuf[i] = complex_t((buf[i * 2 + 0] - 127.0f) / 128.0f, (buf[i * 2 + 1] - 127.0f) / 128.0f);
    stream->swap(len / 2);
};

void RtlSdrSource::set_gains()
{
    if (!is_started)
        return;

    for (int i = 0; i < 20 && rtlsdr_set_agc_mode(rtlsdr_dev_obj, lna_agc_enabled) < 0; i++)
        ;
    for (int i = 0; i < 20 && rtlsdr_set_tuner_gain(rtlsdr_dev_obj, gain * 10) < 0; i++)
        ;

    if (lna_agc_enabled)
    {
        for (int i = 0; i < 20 && rtlsdr_set_tuner_gain_mode(rtlsdr_dev_obj, 0) < 0; i++)
            ;
    }
    else
    {
        for (int i = 0; i < 20 && rtlsdr_set_tuner_gain_mode(rtlsdr_dev_obj, 1) < 0; i++)
            ;
        for (int i = 0; i < 20 && rtlsdr_set_tuner_gain(rtlsdr_dev_obj, gain * 10) < 0; i++)
            ;
    }

    logger->debug("Set RTL-SDR AGC to %d", (int)lna_agc_enabled);
    logger->debug("Set RTL-SDR Gain to %d", gain);
}

void RtlSdrSource::set_bias()
{
    if (!is_started)
        return;
    for (int i = 0; i < 20 && rtlsdr_set_bias_tee(rtlsdr_dev_obj, bias_enabled) < 0; i++)
        ;
    logger->debug("Set RTL-SDR Bias to %d", (int)bias_enabled);
}

void RtlSdrSource::set_ppm()
{
    if (!is_started)
        return;
    int ppm = ppm_widget.get();
    for (int i = 0; i < 20 && rtlsdr_set_freq_correction(rtlsdr_dev_obj, ppm) < 0; i++)
        ;
    logger->debug("Set RTL-SDR PPM Correction to %d", ppm);
}

void RtlSdrSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    gain = getValueOrDefault(d_settings["gain"], gain);
    lna_agc_enabled = getValueOrDefault(d_settings["agc"], lna_agc_enabled);
    bias_enabled = getValueOrDefault(d_settings["bias"], bias_enabled);
    ppm_widget.set(getValueOrDefault(d_settings["ppm_correction"], ppm_widget.get()));

    if (is_started)
    {
        set_bias();
        set_gains();
        set_ppm();
    }
}

nlohmann::json RtlSdrSource::get_settings()
{
    d_settings["gain"] = gain;
    d_settings["agc"] = lna_agc_enabled;
    d_settings["bias"] = bias_enabled;
    d_settings["ppm_correction"] = ppm_widget.get();

    return d_settings;
}

void RtlSdrSource::open()
{
    is_open = true;

    // Set available samplerate
    std::vector<double> available_samplerates;
    available_samplerates.push_back(250000);
    available_samplerates.push_back(1024000);
    available_samplerates.push_back(1536000);
    available_samplerates.push_back(1792000);
    available_samplerates.push_back(1920000);
    available_samplerates.push_back(2048000);
    available_samplerates.push_back(2160000);
    available_samplerates.push_back(2400000);
    available_samplerates.push_back(2560000);
    available_samplerates.push_back(2880000);
    available_samplerates.push_back(3200000);

    samplerate_widget.set_list(available_samplerates, true);
}

void RtlSdrSource::start()
{
    DSPSampleSource::start();
#ifndef __ANDROID__
    if (rtlsdr_open(&rtlsdr_dev_obj, d_sdr_id) != 0)
        throw std::runtime_error("Could not open RTL-SDR device!");
#else
    int vid, pid;
    std::string path;
    int fd = getDeviceFD(vid, pid, RTLSDR_USB_VID_PID, path);
    if (rtlsdr_open_fd(&rtlsdr_dev_obj, fd) != 0)
        throw std::runtime_error("Could not open RTL-SDR device!");
#endif

    uint64_t current_samplerate = samplerate_widget.get_value();

    logger->debug("Set RTL-SDR samplerate to " + std::to_string(current_samplerate));
    rtlsdr_set_sample_rate(rtlsdr_dev_obj, current_samplerate);

    is_started = true;

    set_frequency(d_frequency);

    set_bias();
    set_gains();
    set_ppm();

    rtlsdr_reset_buffer(rtlsdr_dev_obj);

    thread_should_run = true;
    work_thread = std::thread(&RtlSdrSource::mainThread, this);
}

void RtlSdrSource::stop()
{
    if (is_started)
    {
        rtlsdr_cancel_async(rtlsdr_dev_obj);
        thread_should_run = false;
        logger->info("Waiting for the thread...");
        if (is_started)
            output_stream->stopWriter();
        if (work_thread.joinable())
            work_thread.join();
        logger->info("Thread stopped");
        rtlsdr_close(rtlsdr_dev_obj);
    }
    is_started = false;
}

void RtlSdrSource::close()
{
    is_open = false;
}

void RtlSdrSource::set_frequency(uint64_t frequency)
{
    if (is_started)
    {
        for (int i = 0; i < 20 && rtlsdr_set_center_freq(rtlsdr_dev_obj, frequency) < 0; i++)
            ;
        logger->debug("Set RTL-SDR frequency to %d", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void RtlSdrSource::drawControlUI()
{
    if (is_started)
        RImGui::beginDisabled();

    samplerate_widget.render();

    if (is_started)
        RImGui::endDisabled();

    if (ppm_widget.draw())
        set_ppm();

    if (RImGui::SliderInt("LNA Gain", &gain, 0, 49))
        set_gains();

    if (RImGui::Checkbox("AGC", &lna_agc_enabled))
        set_gains();

    if (RImGui::Checkbox("Bias-Tee", &bias_enabled))
        set_bias();
}

void RtlSdrSource::set_samplerate(uint64_t samplerate)
{
    if (!samplerate_widget.set_value(samplerate, 3.2e6))
        throw std::runtime_error("Unspported samplerate : " + std::to_string(samplerate) + "!");
}

uint64_t RtlSdrSource::get_samplerate()
{
    return samplerate_widget.get_value();
}

std::vector<dsp::SourceDescriptor> RtlSdrSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

#ifndef __ANDROID__
    int c = rtlsdr_get_device_count();

    for (int i = 0; i < c; i++)
    {
        const char *name = rtlsdr_get_device_name(i);
        results.push_back({"rtlsdr", std::string(name) + " #" + std::to_string(i), uint64_t(i)});
    }
#else
    int vid, pid;
    std::string path;
    if (getDeviceFD(vid, pid, RTLSDR_USB_VID_PID, path) != -1)
        results.push_back({"rtlsdr", "RTL-SDR USB", 0});
#endif

    return results;
}

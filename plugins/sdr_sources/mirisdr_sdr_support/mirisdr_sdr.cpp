#include "mirisdr_sdr.h"

#ifdef __ANDROID__
#include "common/dsp_sample_source/android_usb_backend.h"

const std::vector<DevVIDPID> MIRISDR_USB_VID_PID = {{0x1df7, 0x2500},
                                                    {0x2040, 0xd300},
                                                    {0x07ca, 0x8591},
                                                    {0x04bb, 0x0537},
                                                    {0x0511, 0x0037}};
#endif

void MiriSdrSource::_rx_callback_8(unsigned char *buf, uint32_t len, void *ctx)
{
    std::shared_ptr<dsp::stream<complex_t>> stream = *((std::shared_ptr<dsp::stream<complex_t>> *)ctx);
    volk_8i_s32f_convert_32f((float *)stream->writeBuf, (int8_t *)buf, 127.0f, len);
    stream->swap(len / 2);
};

void MiriSdrSource::_rx_callback_16(unsigned char *buf, uint32_t len, void *ctx)
{
    std::shared_ptr<dsp::stream<complex_t>> stream = *((std::shared_ptr<dsp::stream<complex_t>> *)ctx);
    volk_16i_s32f_convert_32f((float *)stream->writeBuf, (int16_t *)buf, 32768.0f, len / 2);
    stream->swap(len / 2);
};

void MiriSdrSource::set_gains()
{
    if (!is_started)
        return;

    mirisdr_set_tuner_gain_mode(mirisdr_dev_obj, 1);
    mirisdr_set_tuner_gain(mirisdr_dev_obj, gain * 10);
    logger->debug("Set MiriSDR Gain to {:d}", gain);
}

void MiriSdrSource::set_bias()
{
    if (!is_started)
        return;
    mirisdr_set_bias(mirisdr_dev_obj, bias_enabled);
    logger->debug("Set MiriSDR Bias to {:d}", (int)bias_enabled);
}

void MiriSdrSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    gain = getValueOrDefault(d_settings["gain"], gain);
    bias_enabled = getValueOrDefault(d_settings["bias"], bias_enabled);
    bit_depth = getValueOrDefault(d_settings["bit_depth"], bit_depth);

    if (is_started)
    {
        set_gains();
        set_bias();
    }
}

nlohmann::json MiriSdrSource::get_settings(nlohmann::json)
{
    d_settings["gain"] = gain;
    d_settings["bias"] = bias_enabled;
    d_settings["bit_depth"] = bit_depth;

    return d_settings;
}

void MiriSdrSource::open()
{
    is_open = true;

    // Set available samplerate
    available_samplerates.clear();
    available_samplerates.push_back(2e6);
    available_samplerates.push_back(3e6);
    available_samplerates.push_back(4e6);
    available_samplerates.push_back(5e6);
    available_samplerates.push_back(7e6);
    available_samplerates.push_back(8e6);
    available_samplerates.push_back(9e6);
    available_samplerates.push_back(10e6);

    // Init UI stuff
    samplerate_option_str = "";
    for (uint64_t samplerate : available_samplerates)
        samplerate_option_str += std::to_string(samplerate) + '\0';
}

void MiriSdrSource::start()
{
    DSPSampleSource::start();
#ifndef __ANDROID__
    if (mirisdr_open(&mirisdr_dev_obj, d_sdr_id) != 0)
        throw std::runtime_error("Could not open MiriSDR device!");
#else
    int vid, pid;
    std::string path;
    int fd = getDeviceFD(vid, pid, MIRISDR_USB_VID_PID, path);
    if (mirisdr_open_fd(&mirisdr_dev_obj, 0, fd) != 0)
        throw std::runtime_error("Could not open MiriSDR device!");
#endif

    mirisdr_set_hw_flavour(mirisdr_dev_obj, MIRISDR_HW_DEFAULT);

    logger->debug("Set MiriSDR samplerate to " + std::to_string(current_samplerate));
    mirisdr_set_sample_rate(mirisdr_dev_obj, current_samplerate);

    mirisdr_set_if_freq(mirisdr_dev_obj, 0);                    // ZeroIFu
    mirisdr_set_bandwidth(mirisdr_dev_obj, current_samplerate); // Set BW
    mirisdr_set_transfer(mirisdr_dev_obj, (char *)"BULK");      // Bulk USB transfers

    if (bit_depth == 8)
        mirisdr_set_sample_format(mirisdr_dev_obj, (char *)"504_S8");
    else if (bit_depth == 10)
        mirisdr_set_sample_format(mirisdr_dev_obj, (char *)"384_S16");
    else if (bit_depth == 12)
        mirisdr_set_sample_format(mirisdr_dev_obj, (char *)"336_S16");
    else if (bit_depth == 14)
        mirisdr_set_sample_format(mirisdr_dev_obj, (char *)"252_S16");

    is_started = true;

    set_frequency(d_frequency);

    set_gains();
    set_bias();

    mirisdr_reset_buffer(mirisdr_dev_obj);
    needs_to_run = true;
}

void MiriSdrSource::stop()
{
    needs_to_run = false;
    if (is_started)
    {
        mirisdr_cancel_async(mirisdr_dev_obj);
        while (async_running)
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Wait
        mirisdr_close(mirisdr_dev_obj);
    }
    is_started = false;
}

void MiriSdrSource::close()
{
    is_open = false;
}

void MiriSdrSource::set_frequency(uint64_t frequency)
{
    if (is_started)
    {
        mirisdr_set_center_freq(mirisdr_dev_obj, frequency);
        logger->debug("Set MiriSDR frequency to {:d}", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void MiriSdrSource::drawControlUI()
{
    if (is_started)
        style::beginDisabled();
    ImGui::Combo("Samplerate", &selected_samplerate, samplerate_option_str.c_str());
    current_samplerate = available_samplerates[selected_samplerate];

    if (ImGui::Combo("Bit Depth", &bit_depth_sel, "8-bits\0"
                                                  "10-bits\0"
                                                  "12-bits\0"
                                                  "14-bits\0"))
    {
        if (bit_depth_sel == 0)
            bit_depth = 8;
        else if (bit_depth_sel == 1)
            bit_depth = 10;
        else if (bit_depth_sel == 2)
            bit_depth = 12;
        else if (bit_depth_sel == 3)
            bit_depth = 14;
    }
    if (is_started)
        style::endDisabled();

    if (ImGui::SliderInt("LNA Gain", &gain, 0, 10))
        set_gains();

    if (ImGui::Checkbox("Bias-Tee", &bias_enabled))
        set_bias();
}

void MiriSdrSource::set_samplerate(uint64_t samplerate)
{
    for (int i = 0; i < (int)available_samplerates.size(); i++)
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

uint64_t MiriSdrSource::get_samplerate()
{
    return current_samplerate;
}

std::vector<dsp::SourceDescriptor> MiriSdrSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

#ifndef __ANDROID__
    int c = mirisdr_get_device_count();

    for (int i = 0; i < c; i++)
    {
        const char *name = mirisdr_get_device_name(i);
        results.push_back({"mirisdr", std::string(name) + " #" + std::to_string(i), uint64_t(i)});
    }
#else
    int vid, pid;
    std::string path;
    if (getDeviceFD(vid, pid, MIRISDR_USB_VID_PID, path) != -1)
        results.push_back({"mirisdr", "MiriSDR USB", 0});
#endif

    return results;
}
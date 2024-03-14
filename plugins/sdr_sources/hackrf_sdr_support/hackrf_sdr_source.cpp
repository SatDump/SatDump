#include "hackrf_sdr_source.h"

#ifdef __ANDROID__
#include "common/dsp_source_sink/android_usb_backend.h"

const std::vector<DevVIDPID> HACKRF_USB_VID_PID = {{0x1d50, 0x604b}, {0x1d50, 0x6089}, {0x1d50, 0xcc15}};
#endif

int HackRFSource::_rx_callback(hackrf_transfer *t)
{
    std::shared_ptr<dsp::stream<complex_t>> stream = *((std::shared_ptr<dsp::stream<complex_t>> *)t->rx_ctx);
    for (int i = 0; i < t->buffer_length / 2; i++)
    {
        stream->writeBuf[i].real = ((int8_t *)t->buffer)[i * 2 + 0] / 128.0f;
        stream->writeBuf[i].imag = ((int8_t *)t->buffer)[i * 2 + 1] / 128.0f;
    }
    stream->swap(t->buffer_length / 2);
    return 0;
}

void HackRFSource::set_gains()
{
    if (!is_started)
        return;
    hackrf_set_amp_enable(hackrf_dev_obj, amp_enabled);
    hackrf_set_lna_gain(hackrf_dev_obj, lna_gain);
    hackrf_set_vga_gain(hackrf_dev_obj, vga_gain);
    logger->debug("Set HackRF AMP to %d", (int)amp_enabled);
    logger->debug("Set HackRF LNA gain to %d", lna_gain);
    logger->debug("Set HackRF VGA gain to %d", vga_gain);
}

void HackRFSource::set_bias()
{
    if (!is_started)
        return;
    hackrf_set_antenna_enable(hackrf_dev_obj, bias_enabled);
    logger->debug("Set HackRF bias to %d", (int)bias_enabled);
}

void HackRFSource::set_others()
{
    if (!is_started)
        return;

    int set_to = manual_bandwidth ? bandwidth_widget.get_value() : samplerate_widget.get_value();
    hackrf_set_baseband_filter_bandwidth(hackrf_dev_obj, set_to);
    logger->debug("Set HackRF filter bandwidth to %d", set_to);
}

void HackRFSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    amp_enabled = getValueOrDefault(d_settings["amp"], amp_enabled);
    lna_gain = getValueOrDefault(d_settings["lna_gain"], lna_gain);
    vga_gain = getValueOrDefault(d_settings["vga_gain"], vga_gain);
    manual_bandwidth = getValueOrDefault(d_settings["manual_bw"], manual_bandwidth);
    bandwidth_widget.set_value(getValueOrDefault(d_settings["manual_bw_value"], samplerate_widget.get_value()));

    bias_enabled = getValueOrDefault(d_settings["bias"], bias_enabled);

    if (is_open)
    {
        set_gains();
        set_bias();
        set_others();
    }
}

nlohmann::json HackRFSource::get_settings()
{
    d_settings["amp"] = amp_enabled;
    d_settings["lna_gain"] = lna_gain;
    d_settings["vga_gain"] = vga_gain;
    d_settings["manual_bw"] = manual_bandwidth;
    d_settings["manual_bw_value"] = bandwidth_widget.get_value();

    d_settings["bias"] = bias_enabled;

    return d_settings;
}

void HackRFSource::open()
{
    is_open = true;

    // Set available samplerates
    std::vector<double> available_samplerates;
    for (int i = 1; i < 21; i++)
    {
        available_samplerates.push_back(i * 1e6);
    }
    samplerate_widget.set_list(available_samplerates, true);

    // Set available bandwidths
    std::vector<double> available_bandwidths = {
        1750000,
        2500000,
        3500000,
        5000000,
        5500000,
        6000000,
        7000000,
        8000000,
        9000000,
        10000000,
        12000000,
        14000000,
        15000000,
        20000000,
        24000000,
        28000000};
    bandwidth_widget.set_list(available_bandwidths, false, "Hz");
}

void HackRFSource::start()
{
    DSPSampleSource::start();

#ifndef __ANDROID__
    std::stringstream ss;
    ss << std::hex << d_sdr_id;
    if (hackrf_open_by_serial(ss.str().c_str(), &hackrf_dev_obj) != 0)
        throw satdump_exception("Could not open HackRF device!");
#else
    int vid, pid;
    std::string path;
    int fd = getDeviceFD(vid, pid, HACKRF_USB_VID_PID, path);
    if (hackrf_open_by_fd(&hackrf_dev_obj, fd) != 0)
        throw satdump_exception("Could not open HackRF device!");
#endif

    uint64_t current_samplerate = samplerate_widget.get_value();

    // hackrf_reset(hackrf_dev_obj);

    logger->debug("Set HackRF samplerate to " + std::to_string(current_samplerate));
    hackrf_set_sample_rate(hackrf_dev_obj, current_samplerate);

    is_started = true;

    set_frequency(d_frequency);

    set_others();
    set_gains();
    set_bias();

    hackrf_start_rx(hackrf_dev_obj, &_rx_callback, &output_stream);
}

void HackRFSource::stop()
{
    if (is_started)
    {
        hackrf_set_antenna_enable(hackrf_dev_obj, false);
        hackrf_stop_rx(hackrf_dev_obj);
        hackrf_close(hackrf_dev_obj);
        is_started = false;
    }
}

void HackRFSource::close()
{
    // if (is_open)
}

void HackRFSource::set_frequency(uint64_t frequency)
{
    if (is_open && is_started)
    {
        hackrf_set_freq(hackrf_dev_obj, frequency);
        logger->debug("Set HackRF frequency to %d", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void HackRFSource::drawControlUI()
{
    // Samplerate
    if (is_started)
        RImGui::beginDisabled();

    samplerate_widget.render();

    if (is_started)
        RImGui::endDisabled();

    // Gain settings
    bool gain_changed = false;
    gain_changed |= RImGui::Checkbox("Amp", &amp_enabled);
    gain_changed |= RImGui::SteppedSliderInt("LNA Gain", &lna_gain, 0, 40, 8, "%d", ImGuiSliderFlags_AlwaysClamp);
    gain_changed |= RImGui::SteppedSliderInt("VGA Gain", &vga_gain, 0, 49, 2, "%d", ImGuiSliderFlags_AlwaysClamp);

    if (gain_changed)
        set_gains();

    if (RImGui::Checkbox("Bias-Tee", &bias_enabled))
        set_bias();

    // Bandwidth Filter
    bool bw_update = RImGui::Checkbox("Manual Bandwidth", &manual_bandwidth);
    if (manual_bandwidth)
        bw_update = bw_update || bandwidth_widget.render();

    if (bw_update && is_started)
        set_others();
}

void HackRFSource::set_samplerate(uint64_t samplerate)
{
    if (!samplerate_widget.set_value(samplerate, 40e6))
        throw satdump_exception("Unspported samplerate : " + std::to_string(samplerate) + "!");
}

uint64_t HackRFSource::get_samplerate()
{
    return samplerate_widget.get_value();
}

std::vector<dsp::SourceDescriptor> HackRFSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

#ifndef __ANDROID__
    hackrf_device_list_t *devlist = hackrf_device_list();

    for (int i = 0; i < devlist->devicecount; i++)
    {
        if (devlist->serial_numbers[i] == nullptr)
            results.push_back({"hackrf", "HackRF One [In Use]", 0});
        else
        {
            std::stringstream ss;
            uint64_t id = 0;
            ss << devlist->serial_numbers[i];
            ss >> std::hex >> id;
            ss << devlist->serial_numbers[i];
            results.push_back({"hackrf", "HackRF One " + ss.str().substr(16, 16), id});
        }
    }

    hackrf_device_list_free(devlist);
#else
    int vid, pid;
    std::string path;
    if (getDeviceFD(vid, pid, HACKRF_USB_VID_PID, path) != -1)
        results.push_back({"hackrf", "HackRF One USB", 0});
#endif

    return results;
}
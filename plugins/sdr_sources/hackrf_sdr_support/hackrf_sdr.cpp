#include "hackrf_sdr.h"

int HackRFSource::_rx_callback(hackrf_transfer *t)
{
    std::shared_ptr<dsp::stream<complex_t>> stream = *((std::shared_ptr<dsp::stream<complex_t>> *)t->rx_ctx);
    for (int i = 0; i < t->buffer_length / 2; i++)
        stream->writeBuf[i] = complex_t(((int8_t *)t->buffer)[i * 2 + 0] / 128.0f, ((int8_t *)t->buffer)[i * 2 + 1] / 128.0f);
    stream->swap(t->buffer_length / 2);
    return 0;
}

void HackRFSource::set_gains()
{
    hackrf_set_amp_enable(hackrf_dev_obj, amp_enabled);
    hackrf_set_lna_gain(hackrf_dev_obj, lna_gain);
    hackrf_set_vga_gain(hackrf_dev_obj, vga_gain);
    logger->debug("Set HackRF AMP to {:d}", (int)amp_enabled);
    logger->debug("Set HackRF LNA gain to {:d}", lna_gain);
    logger->debug("Set HackRF VGA gain to {:d}", vga_gain);
}

void HackRFSource::set_bias()
{
    hackrf_set_antenna_enable(hackrf_dev_obj, bias_enabled);
    logger->debug("Set HackRF bias to {:d}", (int)bias_enabled);
}

void HackRFSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    amp_enabled = getValueOrDefault(d_settings["amp"], amp_enabled);
    lna_gain = getValueOrDefault(d_settings["lna_gain"], lna_gain);
    vga_gain = getValueOrDefault(d_settings["vga_gain"], vga_gain);

    bias_enabled = getValueOrDefault(d_settings["bias"], bias_enabled);

    if (is_open)
    {
        set_gains();
        set_bias();
    }
}

nlohmann::json HackRFSource::get_settings(nlohmann::json)
{
    d_settings["amp"] = amp_enabled;
    d_settings["lna_gain"] = lna_gain;
    d_settings["vga_gain"] = vga_gain;

    d_settings["bias"] = bias_enabled;

    return d_settings;
}

void HackRFSource::open()
{
    std::stringstream ss;
    ss << std::hex << d_sdr_id;
    if (!is_open)
        if (hackrf_open_by_serial(ss.str().c_str(), &hackrf_dev_obj) != 0)
            throw std::runtime_error("Could not open HackRF device!");
    is_open = true;

    // hackrf_reset(hackrf_dev_obj);

    // Set available samplerates
    for (int i = 1; i < 21; i++)
        available_samplerates.push_back(i * 1e6);

    // Init UI stuff
    samplerate_option_str = "";
    for (uint64_t samplerate : available_samplerates)
        samplerate_option_str += std::to_string(samplerate) + '\0';
}

void HackRFSource::start()
{
    DSPSampleSource::start();

    logger->debug("Set HackRF samplerate to " + std::to_string(current_samplerate));
    hackrf_set_sample_rate(hackrf_dev_obj, current_samplerate);
    hackrf_set_baseband_filter_bandwidth(hackrf_dev_obj, current_samplerate);

    set_frequency(d_frequency);

    set_gains();
    set_bias();

    hackrf_start_rx(hackrf_dev_obj, &_rx_callback, &output_stream);

    is_started = true;
}

void HackRFSource::stop()
{
    hackrf_stop_rx(hackrf_dev_obj);
    is_started = false;
}

void HackRFSource::close()
{
    if (is_open)
        hackrf_close(hackrf_dev_obj);
}

void HackRFSource::set_frequency(uint64_t frequency)
{
    if (is_open)
    {
        hackrf_set_freq(hackrf_dev_obj, frequency);
        logger->debug("Set HackRF frequency to {:d}", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void HackRFSource::drawControlUI()
{
    if (is_started)
        style::beginDisabled();
    ImGui::Combo("Samplerate", &selected_samplerate, samplerate_option_str.c_str());
    current_samplerate = available_samplerates[selected_samplerate];
    if (is_started)
        style::endDisabled();

    // Gain settings
    bool gain_changed = false;
    gain_changed |= ImGui::Checkbox("Amp", &amp_enabled);
    gain_changed |= ImGui::SliderInt("LNA Gain", &lna_gain, 0, 49);
    gain_changed |= ImGui::SliderInt("VGA Gain", &vga_gain, 0, 49);

    if (gain_changed)
        set_gains();

    if (ImGui::Checkbox("Bias-Tee", &bias_enabled))
        set_bias();
}

void HackRFSource::set_samplerate(uint64_t samplerate)
{
    for (int i = 0; i < available_samplerates.size(); i++)
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

uint64_t HackRFSource::get_samplerate()
{
    return current_samplerate;
}

std::vector<dsp::SourceDescriptor> HackRFSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    hackrf_device_list_t *devlist = hackrf_device_list();

    for (int i = 0; i < devlist->devicecount; i++)
    {
        std::stringstream ss;
        uint64_t id = 0;
        ss << devlist->serial_numbers[i];
        ss >> std::hex >> id;
        ss << devlist->serial_numbers[i];
        results.push_back({"hackrf", "HackRF One " + ss.str(), id});
    }

    return results;
}
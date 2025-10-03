#include "hackrf_sdr_sink.h"

int HackRFSink::_tx_callback(hackrf_transfer *t)
{
    ((dsp::RingBuffer<uint8_t> *)t->tx_ctx)->read(t->buffer, t->valid_length);
    return 0;
}

void HackRFSink::set_gains()
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

void HackRFSink::set_bias()
{
    if (!is_started)
        return;
    hackrf_set_antenna_enable(hackrf_dev_obj, bias_enabled);
    logger->debug("Set HackRF bias to %d", (int)bias_enabled);
}

void HackRFSink::set_others()
{
    if (!is_started)
        return;

    uint64_t set_to = manual_bandwidth ? manual_bw_value : current_samplerate;
    hackrf_set_baseband_filter_bandwidth(hackrf_dev_obj, (uint32_t)set_to);
    logger->debug("Set HackRF filter bandwidth to %" PRIu64, set_to);
}

void HackRFSink::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    amp_enabled = getValueOrDefault(d_settings["amp"], amp_enabled);
    lna_gain = getValueOrDefault(d_settings["lna_gain"], lna_gain);
    vga_gain = getValueOrDefault(d_settings["vga_gain"], vga_gain);
    manual_bandwidth = getValueOrDefault(d_settings["manual_bw"], manual_bandwidth);
    bias_enabled = getValueOrDefault(d_settings["bias"], bias_enabled);

    manual_bw_value = getValueOrDefault(d_settings["manual_bw_value"], current_samplerate);
    for (int i = 0; i < (int)available_bandwidths.size(); i++)
    {
        if (manual_bw_value == available_bandwidths[i])
        {
            selected_bw = i;
            break;
        }
    }

    if (is_open)
    {
        set_gains();
        set_bias();
        set_others();
    }
}

nlohmann::json HackRFSink::get_settings()
{
    d_settings["amp"] = amp_enabled;
    d_settings["lna_gain"] = lna_gain;
    d_settings["vga_gain"] = vga_gain;
    d_settings["manual_bw"] = manual_bandwidth;
    d_settings["manual_bw_value"] = manual_bw_value;

    d_settings["bias"] = bias_enabled;

    return d_settings;
}

void HackRFSink::open()
{
    is_open = true;

    // Set available samplerates
    for (int i = 1; i < 21; i++)
    {
        available_samplerates.push_back(i * 1e6);
        available_samplerates_exp.push_back(i * 1e6);
    }

    for (int i = 21; i < 38; i++)
        available_samplerates_exp.push_back(i * 1e6);

    // Set available bandwidths
    available_bandwidths = {
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

    // Init UI stuff
    bandwidth_option_str = samplerate_option_str = samplerate_option_str_exp = "";
    for (uint64_t bandwidth : available_bandwidths)
        bandwidth_option_str += format_notated(bandwidth, "Hz") + '\0';
    for (uint64_t samplerate : available_samplerates)
        samplerate_option_str += format_notated(samplerate, "sps") + '\0';
    for (uint64_t samplerate : available_samplerates_exp)
        samplerate_option_str_exp += format_notated(samplerate, "sps") + '\0';
}

void HackRFSink::start(std::shared_ptr<dsp::stream<complex_t>> stream)
{
    DSPSampleSink::start(stream);

    if (hackrf_open_by_serial(d_sdr_id.c_str(), &hackrf_dev_obj) != 0)
        throw satdump_exception("Could not open HackRF device!");

    // hackrf_reset(hackrf_dev_obj);

    logger->debug("Set HackRF samplerate to " + std::to_string(current_samplerate));
    hackrf_set_sample_rate(hackrf_dev_obj, current_samplerate);

    is_started = true;

    set_frequency(d_frequency);

    set_others();
    set_gains();
    set_bias();

    hackrf_start_tx(hackrf_dev_obj, &_tx_callback, &fifo_out);
    should_run = true;
}

void HackRFSink::stop()
{
    DSPSampleSink::stop();
    should_run = false;
    if (is_started)
    {
        hackrf_stop_tx(hackrf_dev_obj);
        hackrf_close(hackrf_dev_obj);
        is_started = false;
    }
}

void HackRFSink::close()
{
    // if (is_open)
}

void HackRFSink::set_frequency(uint64_t frequency)
{
    if (is_open && is_started)
    {
        hackrf_set_freq(hackrf_dev_obj, frequency);
        logger->debug("Set HackRF frequency to %d", frequency);
    }
    DSPSampleSink::set_frequency(frequency);
}

void HackRFSink::drawControlUI()
{
    if (is_started)
        style::beginDisabled();
    ImGui::Combo("Samplerate", &selected_samplerate, enable_experimental_samplerates ? samplerate_option_str_exp.c_str() : samplerate_option_str.c_str());
    current_samplerate = enable_experimental_samplerates ? available_samplerates_exp[selected_samplerate] : available_samplerates[selected_samplerate];
    ImGui::Checkbox("Exp. Samplerates", &enable_experimental_samplerates);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Enable unsupported samplerates.\nThe HackRF can (normally) also run at those,\nbut not without sampledrops.\nHence, they are mostly good for experiments.");
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

    bool bw_update = ImGui::Checkbox("Manual Bandwidth", &manual_bandwidth);
    if (manual_bandwidth)
    {
        bw_update = bw_update || ImGui::Combo("Bandwidth", &selected_bw, bandwidth_option_str.c_str());
        if (bw_update)
            manual_bw_value = available_bandwidths[selected_bw];
    }
    if (bw_update)
        set_others();
}

void HackRFSink::set_samplerate(uint64_t samplerate)
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

    throw satdump_exception("Unsupported samplerate : " + std::to_string(samplerate) + "!");
}

uint64_t HackRFSink::get_samplerate()
{
    return current_samplerate;
}

std::vector<dsp::SinkDescriptor> HackRFSink::getAvailableSinks()
{
    std::vector<dsp::SinkDescriptor> results;

    hackrf_device_list_t *devlist = hackrf_device_list();

    if (devlist != nullptr)
    {
        for (int i = 0; i < devlist->devicecount; i++)
        {
            if (devlist->serial_numbers[i] == nullptr)
                results.push_back({"hackrf", "HackRF One [In Use]", 0});
            else
            {
                results.push_back({"hackrf", "HackRF One " + std::string(devlist->serial_numbers[i]), std::string(devlist->serial_numbers[i])});
            }
        }

        hackrf_device_list_free(devlist);
    }

    return results;
}

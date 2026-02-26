#include "usrp_sdr.h"

void USRPSource::set_gains()
{
    if (!is_started)
        return;

    usrp_device->set_rx_gain(gain, channel);
    logger->debug("Set USRP gain to %f", gain);
}

void USRPSource::open_sdr()
{
    uhd::device_addrs_t devlist = uhd::device::find(uhd::device_addr_t());

    uhd::device_addr_t addr = devlist[std::stoi(d_sdr_id)];

    // USB transport parameters, optimal values from testing
    addr["recv_frame_size"] = "8000"; // RX frame size
    addr["num_recv_frames"] = "1900"; // RX buffer size
    // addr["send_frame_size"] = "8000";      // TX frame size
    // addr["num_send_frames"] = "1900";      // TX buffer size

    usrp_device = uhd::usrp::multi_usrp::make(addr);

    // uhd::meta_range_t master_clock_range = usrp_device->get_master_clock_rate_range();
    // usrp_device->set_master_clock_rate(master_clock_range.stop());

    uhd::usrp::subdev_spec_t sub_boards = usrp_device->get_rx_subdev_spec();
    channel_option_str = "";
    for (int i = 0; i < (int)sub_boards.size(); i++)
    {
        logger->trace("USRP has " + usrp_device->get_rx_subdev_name(i) + " in slot " + sub_boards[i].db_name);
        channel_option_str += usrp_device->get_rx_subdev_name(i) + " (" + sub_boards[i].db_name + ")" + '\0';
    }
}

void USRPSource::open_channel()
{
    if (channel >= (int)usrp_device->get_rx_num_channels())
        throw satdump_exception("Channel " + std::to_string(channel) + " is invalid!");

    logger->info("Using USRP channel %d", channel);

    if (usrp_device->get_master_clock_rate_range().start() != usrp_device->get_master_clock_rate_range().stop())
        use_device_rates = true;

    std::vector<double> available_samplerates;

    // Preserve current samplerate selection before updating the list
    double current_samplerate = samplerate_widget.get_value();

    if (use_device_rates)
    {
        // Get samplerates
        uhd::meta_range_t dev_samplerates = usrp_device->get_master_clock_rate_range();
        available_samplerates.clear();
        for (auto &sr : dev_samplerates)
        {
            if (sr.step() == 0 && sr.start() == sr.stop())
            {
                available_samplerates.push_back(sr.start());
            }
            else if (sr.step() == 0)
            {
                for (double s = std::max(sr.start(), 1e6); s < sr.stop(); s += 1e6)
                    available_samplerates.push_back(s);
                available_samplerates.push_back(sr.stop());
            }
            else
            {
                for (double s = sr.start(); s <= sr.stop(); s += sr.step())
                    available_samplerates.push_back(s);
            }
        }
    }
    else
    {
        // Get samplerates
        uhd::meta_range_t dev_samplerates = usrp_device->get_rx_rates(channel);
        available_samplerates.clear();
        for (auto &sr : dev_samplerates)
        {
            if (sr.step() == 0 || sr.start() == sr.stop())
            {
                available_samplerates.push_back(sr.start());
            }
            else
            {
                for (double s = sr.start(); s <= sr.stop(); s += sr.step())
                    available_samplerates.push_back(s);
            }
        }
    }

    samplerate_widget.set_list(available_samplerates, false);

    // Restore the previously selected samplerate if it's still available
    samplerate_widget.set_value(current_samplerate, 0);

    // Get gain range
    gain_range = usrp_device->get_rx_gain_range(channel);

    usrp_antennas = usrp_device->get_rx_antennas(channel);
    antenna_option_str = "";
    for (int i = 0; i < (int)usrp_antennas.size(); i++)
    {
        antenna_option_str += usrp_antennas[i] + '\0';
        logger->trace("USRP has antenna option %s", usrp_antennas[i].c_str());
    }
}

void USRPSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    channel = getValueOrDefault(d_settings["channel"], channel);
    antenna = getValueOrDefault(d_settings["antenna"], antenna);
    gain = getValueOrDefault(d_settings["gain"], gain);
    bit_depth = getValueOrDefault(d_settings["bit_depth"], bit_depth);

    if (bit_depth == 8)
        selected_bit_depth = 0;
    else if (bit_depth == 16)
        selected_bit_depth = 1;

    if (is_started)
    {
        set_gains();
    }
}

nlohmann::json USRPSource::get_settings()
{

    d_settings["channel"] = channel;
    d_settings["antenna"] = antenna;
    d_settings["gain"] = gain;
    d_settings["bit_depth"] = bit_depth;

    return d_settings;
}

void USRPSource::open()
{
    open_sdr();
    is_open = true;
    open_channel();
    usrp_device.reset();
}

void USRPSource::start()
{
    DSPSampleSource::start();
    open_sdr();
    if (!is_open) {
        open_channel();
    }

    uint64_t current_samplerate = samplerate_widget.get_value();

    logger->debug("Set USRP samplerate to " + std::to_string(current_samplerate));
    if (use_device_rates)
        usrp_device->set_master_clock_rate(current_samplerate);
    usrp_device->set_rx_rate(current_samplerate, channel);
    usrp_device->set_rx_bandwidth(current_samplerate, channel);

    if (antenna >= (int)usrp_device->get_rx_antennas(channel).size())
        throw satdump_exception("Antenna " + std::to_string(antenna) + " is invalid!");

    usrp_device->set_rx_antenna(usrp_antennas[antenna], channel);

    is_started = true;

    set_frequency(d_frequency);

    set_gains();

    uhd::stream_args_t sargs;
    sargs.channels.clear();
    sargs.channels.push_back(channel);
    sargs.cpu_format = "fc32";
    if (bit_depth == 8)
        sargs.otw_format = "sc8";
    else if (bit_depth == 16)
        sargs.otw_format = "sc16";

    usrp_streamer = usrp_device->get_rx_stream(sargs);

    thread_should_run = true;
    work_thread = std::thread(&USRPSource::mainThread, this);

    usrp_streamer->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
}

void USRPSource::stop()
{
    thread_should_run = false;
    logger->info("Waiting for the thread...");
    if (is_started)
        output_stream->stopWriter();
    if (work_thread.joinable())
        work_thread.join();
    logger->info("Thread stopped");
    if (is_started)
    {
        usrp_streamer->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
        usrp_streamer.reset();
        usrp_device.reset();
    }
    is_started = false;
}

void USRPSource::close() { is_open = false; }

void USRPSource::set_frequency(uint64_t frequency)
{
    if (is_started)
    {
        usrp_device->set_rx_freq(frequency, channel);
        logger->debug("Set USRP frequency to %d", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void USRPSource::drawControlUI()
{
    if (is_started)
        RImGui::beginDisabled();

    if (RImGui::Combo("Channel", &channel, channel_option_str.c_str()) && is_open)
    {
        open_sdr();
        open_channel();
        usrp_streamer.reset();
        usrp_device.reset();
    }

    RImGui::Combo("Antenna", &antenna, antenna_option_str.c_str());

    samplerate_widget.render();

    if (RImGui::Combo("Bit depth", &selected_bit_depth,
                      "8-bits\0"
                      "16-bits\0"))
    {
        if (selected_bit_depth == 0)
            bit_depth = 8;
        else if (selected_bit_depth == 1)
            bit_depth = 16;
    }

    if (is_started)
        RImGui::endDisabled();

    // Gain settings
    if (is_open)
    {
        if (RImGui::SteppedSliderFloat("Gain", &gain, gain_range.start(), gain_range.stop()))
            set_gains();
    }
    else // Defaults for using UI when device is not attached (config export)
        RImGui::SteppedSliderFloat("Gain", &gain, 0, 60);
}

void USRPSource::set_samplerate(uint64_t samplerate)
{
    if (!samplerate_widget.set_value(samplerate, 0))
        throw satdump_exception("Unsupported samplerate : " + std::to_string(samplerate) + "!");
}

uint64_t USRPSource::get_samplerate() { return samplerate_widget.get_value(); }

std::vector<dsp::SourceDescriptor> USRPSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    uhd::device_addrs_t devlist = uhd::device::find(uhd::device_addr_t());

    uint64_t i = 0;
    for (const uhd::device_addr_t &dev : devlist)
    {
        std::string type = dev.has_key("product") ? dev["product"] : dev["type"];
        results.push_back({"usrp", "USRP " + type + " " + dev["serial"], std::to_string(i)});
        i++;
    }

    return results;
}

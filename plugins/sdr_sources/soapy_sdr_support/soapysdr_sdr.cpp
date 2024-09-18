#include "soapysdr_sdr.h"

// void SoapySdrSource::_rx_callback(unsigned char *buf, uint32_t len, void *ctx)
//{
//     std::shared_ptr<dsp::stream<complex_t>> stream = *((std::shared_ptr<dsp::stream<complex_t>> *)ctx);
//     for (int i = 0; i < (int)len / 2; i++)
//         stream->writeBuf[i] = complex_t((buf[i * 2 + 0] - 127.0f) / 128.0f, (buf[i * 2 + 1] - 127.0f) / 128.0f);
//     stream->swap(len / 2);
// };

void SoapySdrSource::set_params()
{
    if (!is_started)
        return;

    // Antenna
    soapy_dev->setAntenna(SOAPY_SDR_RX, channel_id, antenna_list[antenna_id]);
    logger->debug("Set SoapySDR Antenna to " + antenna_list[antenna_id]);

    // Bandwidth Option
    if (manual_bandwidth)
        soapy_dev->setBandwidth(SOAPY_SDR_RX, channel_id, bandwidth_widget.get_value());
    else if (bandwidth_widget.get_list().size() > 1)
        soapy_dev->setBandwidth(SOAPY_SDR_RX, channel_id, samplerate_widget.get_value());

    // Bool options
    for (int optid = 0; optid < list_options_bool.size(); optid++)
    {
        soapy_dev->writeSetting(list_options_bool_ids[optid], list_options_bool_vals[optid] ? "true" : "false");
        logger->debug("Set SoapySDR %s setting to %s", list_options_bool_ids[optid].c_str(), list_options_bool_vals[optid] ? "true" : "false");
    }

    // String options
    for (int optid = 0; optid < list_options_strings.size(); optid++)
    {
        soapy_dev->writeSetting(list_options_strings_ids[optid], list_options_strings_options[optid][list_options_strings_values[optid]]);
        logger->debug("Set SoapySDR %s setting to %s", list_options_strings_ids[optid].c_str(), list_options_strings_options[optid][list_options_strings_values[optid]].c_str());
    }

    // Gains
    for (int gainid = 0; gainid < gain_list.size(); gainid++)
    {
        soapy_dev->setGain(SOAPY_SDR_RX, channel_id, gain_list[gainid], gain_list_vals[gainid]);
        logger->debug("Set SoapySDR %s Gain to %f", gain_list[gainid].c_str(), gain_list_vals[gainid]);
    }
}

void SoapySdrSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    antenna_id = getValueOrDefault(d_settings["antenna_id"], antenna_id);
    for (int gainid = 0; gainid < gain_list.size(); gainid++)
        gain_list_vals[gainid] = getValueOrDefault(d_settings["gain_" + gain_list[gainid]], gain_list_vals[gainid]);
    if (bandwidth_widget.get_list().size() > 1)
        bandwidth_widget.set_value(getValueOrDefault(d_settings["manual_bw_value"], samplerate_widget.get_value()));
    for (int optid = 0; optid < list_options_bool.size(); optid++)
        list_options_bool_vals[optid] = getValueOrDefault(d_settings[list_options_bool_ids[optid]], list_options_bool_vals[optid]);
    for (int optid = 0; optid < list_options_strings.size(); optid++)
        list_options_strings_values[optid] = getValueOrDefault(d_settings[list_options_strings_ids[optid]], list_options_strings_values[optid]);

    if (is_started)
    {
        set_params();
    }
}

nlohmann::json SoapySdrSource::get_settings()
{
    d_settings["antenna_id"] = antenna_id;
    for (int gainid = 0; gainid < gain_list.size(); gainid++)
        d_settings["gain_" + gain_list[gainid]] = gain_list_vals[gainid];
    if (bandwidth_widget.get_list().size() > 1)
        d_settings["manual_bw_value"] = bandwidth_widget.get_value();
    for (int optid = 0; optid < list_options_bool.size(); optid++)
        d_settings[list_options_bool_ids[optid]] = list_options_bool_vals[optid];
    for (int optid = 0; optid < list_options_strings.size(); optid++)
        d_settings[list_options_strings_ids[optid]] = list_options_strings_values[optid];

    return d_settings;
}

void SoapySdrSource::open()
{
    is_open = true;

    soapy_dev = get_device(d_source_label);

    // Antennas
    antenna_list = soapy_dev->listAntennas(SOAPY_SDR_RX, channel_id);
    txt_antenna_list = "";
    for (auto &v : antenna_list)
        txt_antenna_list += v + '\0';

    // Bandwidth
    std::vector<double> bandwidth_list;
    SoapySDR::RangeList bandwidths = soapy_dev->getBandwidthRange(SOAPY_SDR_RX, channel_id);
    for (auto bw : bandwidths)
        bandwidth_list.push_back(bw.minimum());
    bandwidth_widget.set_list(bandwidth_list, false);

    // Gains
    gain_list = soapy_dev->listGains(SOAPY_SDR_RX, channel_id);
    gain_list_ranges.clear();
    for (auto &v : gain_list)
        gain_list_ranges.push_back(soapy_dev->getGainRange(SOAPY_SDR_RX, channel_id, v));
    gain_list_vals = std::vector<float>(gain_list.size(), 0);

    // Set available samplerates
    samplerate_widget.set_list(soapy_dev->listSampleRates(SOAPY_SDR_RX, channel_id), false);

    // List options
    SoapySDR::ArgInfoList options = soapy_dev->getSettingInfo();
    int i = 0;

    list_options_bool.clear();
    list_options_bool_ids.clear();

    list_options_strings.clear();
    list_options_strings_ids.clear();
    list_options_strings_options.clear();
    list_options_strings_options_txt.clear();
    list_options_strings_values.clear();

    for (auto o : options)
    {
        if (o.type == SoapySDR::ArgInfo::BOOL)
        {
            list_options_bool.push_back(o.name);
            list_options_bool_ids.push_back(o.key);
            list_options_bool_vals[i++] = o.value == "true";
        }
        else if (o.type == SoapySDR::ArgInfo::STRING && o.options.size() > 0)
        {
            list_options_strings.push_back(o.name);
            list_options_strings_ids.push_back(o.key);
            std::string all_opts = "";
            for (auto oo : o.options)
                all_opts += oo + '\0';
            list_options_strings_options_txt.push_back(all_opts);
            list_options_strings_options.push_back(o.options);
            list_options_strings_values.push_back(std::stoi(o.value));
        }
    }

    SoapySDR::Device::unmake(soapy_dev);
}

void SoapySdrSource::start()
{
    DSPSampleSource::start();
    soapy_dev = get_device(d_source_label);
    if (soapy_dev == nullptr)
        throw satdump_exception("Could not open SoapySDR device!");

    uint64_t current_samplerate = samplerate_widget.get_value();

    logger->debug("Set SoapySDR samplerate to " + std::to_string(current_samplerate));
    soapy_dev->setSampleRate(SOAPY_SDR_RX, channel_id, current_samplerate);

    is_started = true;

    set_frequency(d_frequency);

    set_params();

    soapy_dev_stream = soapy_dev->setupStream(SOAPY_SDR_RX, "CF32");
    soapy_dev->activateStream(soapy_dev_stream);

    thread_should_run = true;
    work_thread = std::thread(&SoapySdrSource::mainThread, this);
}

void SoapySdrSource::stop()
{
    if (is_started)
    {
        soapy_dev->deactivateStream(soapy_dev_stream);
        thread_should_run = false;
        logger->info("Waiting for the thread...");
        if (is_started)
            output_stream->stopWriter();
        if (work_thread.joinable())
            work_thread.join();
        logger->info("Thread stopped");
        soapy_dev->closeStream(soapy_dev_stream);
        SoapySDR::Device::unmake(soapy_dev);
    }
    is_started = false;
}

void SoapySdrSource::close()
{
    is_open = false;
}

void SoapySdrSource::set_frequency(uint64_t frequency)
{
    if (is_started)
    {
        soapy_dev->setFrequency(SOAPY_SDR_RX, channel_id, frequency);
        logger->debug("Set SoapySDR frequency to %d", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void SoapySdrSource::drawControlUI()
{
    if (is_started)
        RImGui::beginDisabled();

    samplerate_widget.render();
    RImGui::Combo("Antenna", &antenna_id, txt_antenna_list.c_str());

    if (is_started)
        RImGui::endDisabled();

    bool bw_update = ImGui::Checkbox("Manual Bandwidth", &manual_bandwidth);
    if (manual_bandwidth && bandwidth_widget.get_list().size() > 1)
        bw_update |= bandwidth_widget.render();
    if (bw_update)
        set_params();

    for (int gainid = 0; gainid < gain_list.size(); gainid++)
    {
        std::string name = gain_list[gainid] + " Gain";
        if (RImGui::SteppedSliderFloat(name.c_str(),
                                       &gain_list_vals[gainid],
                                       gain_list_ranges[gainid].minimum(),
                                       gain_list_ranges[gainid].maximum(),
                                       gain_list_ranges[gainid].step()))
            set_params();
    }

    for (int optid = 0; optid < list_options_bool.size(); optid++)
    {
        std::string name = list_options_bool[optid];
        if (RImGui::Checkbox(name.c_str(), &list_options_bool_vals[optid]))
            set_params();
    }

    for (int optid = 0; optid < list_options_strings.size(); optid++)
    {
        std::string name = list_options_strings[optid];
        if (RImGui::Combo(name.c_str(), &list_options_strings_values[optid], list_options_strings_options_txt[optid].c_str()))
            set_params();
    }
}

void SoapySdrSource::set_samplerate(uint64_t samplerate)
{
    if (!samplerate_widget.set_value(samplerate))
        throw satdump_exception("Unsupported samplerate : " + std::to_string(samplerate) + "!");
}

uint64_t SoapySdrSource::get_samplerate()
{
    return samplerate_widget.get_value();
}

std::vector<dsp::SourceDescriptor> SoapySdrSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    auto devices = SoapySDR::Device::enumerate();

    int i = 0;
    for (auto &dev : devices)
    {
        std::string name = (dev["label"] != "" ? dev["label"] : dev["driver"]) + " [Soapy]";
        results.push_back({"soapysdr", name, std::to_string(i++)});
    }

    return results;
}

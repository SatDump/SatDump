#include "rfnm_sdr.h"

void RFNMSource::set_gains()
{
    if (!is_started)
        return;

    rfnm_dev_obj->set_rx_channel_gain(channel, gain);
    rfnm_api_failcode r = rfnm_dev_obj->set(channel == 1 ? rfnm::APPLY_CH1_RX : rfnm::APPLY_CH0_RX);
    if (r != rfnm_api_failcode::RFNM_API_OK)
        logger->error("RFNM Error %d", r);
    logger->debug("Set RFNM Gain to %d", gain);
}

void RFNMSource::set_others()
{
    if (!is_started)
        return;

    rfnm_dev_obj->set_rx_channel_rfic_lpf_bw(channel, bandwidth_widget.get_value() / 1e6);
    rfnm_dev_obj->set_rx_channel_fm_notch(channel, fmnotch_enabled ? rfnm_fm_notch::RFNM_FM_NOTCH_ON : rfnm_fm_notch::RFNM_FM_NOTCH_OFF);
    rfnm_dev_obj->set_rx_channel_bias_tee(channel, bias_enabled ? rfnm_bias_tee::RFNM_BIAS_TEE_ON : rfnm_bias_tee::RFNM_BIAS_TEE_OFF);

    rfnm_api_failcode r = rfnm_dev_obj->set(channel == 1 ? rfnm::APPLY_CH1_RX : rfnm::APPLY_CH0_RX);
    if (r != rfnm_api_failcode::RFNM_API_OK)
        logger->error("RFNM Error %d", r);
    logger->debug("Set RFNM BW to %d", int(bandwidth_widget.get_value() / 1e6));
    logger->debug("Set RFNM FM Notch to %d", int(fmnotch_enabled));
    logger->debug("Set RFNM Bias-Tee to %d", int(bias_enabled));
}

void RFNMSource::open_sdr()
{
    rfnm_dev_obj = new rfnm::device(rfnm::TRANSPORT_USB, d_sdr_id);
}

void RFNMSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    channel = getValueOrDefault(d_settings["channel"], channel);
    gain = getValueOrDefault(d_settings["gain"], gain);
    fmnotch_enabled = getValueOrDefault(d_settings["fmnotch"], fmnotch_enabled);
    bias_enabled = getValueOrDefault(d_settings["bias"], bias_enabled);
    bandwidth_widget.set_value(getValueOrDefault(d_settings["lna_agc"], 100e6));
    corr_mag = getValueOrDefault(d_settings["iq_corr_mag"], corr_mag);
    corr_phase = getValueOrDefault(d_settings["iq_corr_phase"], corr_phase);

    if (is_started)
    {
        set_gains();
        set_others();
    }
}

nlohmann::json RFNMSource::get_settings()
{
    d_settings["channel"] = channel;
    d_settings["gain"] = gain;
    d_settings["bandwidth"] = bandwidth_widget.get_value();
    d_settings["fmnotch"] = fmnotch_enabled;
    d_settings["bias"] = bias_enabled;
    d_settings["iq_corr_mag"] = corr_mag;
    d_settings["iq_corr_phase"] = corr_phase;

    return d_settings;
}

void RFNMSource::open()
{
    open_sdr();
    is_open = true;

    // Set available samplerate
    std::vector<double> available_samplerates;
    available_samplerates.push_back(rfnm_dev_obj->get_hwinfo()->clock.dcs_clk);
    available_samplerates.push_back(rfnm_dev_obj->get_hwinfo()->clock.dcs_clk / 2);
    samplerate_widget.set_list(available_samplerates, false);

    // Set available bandwidth
    std::vector<double> available_bandwidth;
    for (int i = 1; i <= 100; i++)
        available_bandwidth.push_back(i * 1e6);
    bandwidth_widget.set_list(available_bandwidth, false);

    //    min_gain = rfnm_dev_obj->s->rx.ch[channel].gain_range.min;
    //    max_gain = rfnm_dev_obj->s->rx.ch[channel].gain_range.max;
    if (min_gain == 0)
        min_gain = -60;
    if (max_gain == 0)
        max_gain = 60;

    delete rfnm_dev_obj;
}

void RFNMSource::start()
{
    DSPSampleSource::start();
    open_sdr();

    uint64_t current_samplerate = samplerate_widget.get_value();

    //    rfnm_dev_obj->set_rx_channel_active(0, RFNM_CH_OFF, RFNM_CH_STREAM_AUTO);
    //    rfnm_dev_obj->set_rx_channel_active(1, RFNM_CH_OFF, RFNM_CH_STREAM_AUTO);

    if (current_samplerate == rfnm_dev_obj->get_hwinfo()->clock.dcs_clk / 2)
        rfnm_dev_obj->set_rx_channel_samp_freq_div(channel, 1, 2);
    else
        rfnm_dev_obj->set_rx_channel_samp_freq_div(channel, 1, 1);

    rfnm_dev_obj->set_rx_channel_path(channel, rfnm_dev_obj->get_rx_channel(channel)->path_preferred);
    rfnm_api_failcode r = rfnm_dev_obj->set(channel == 1 ? rfnm::APPLY_CH1_RX : rfnm::APPLY_CH0_RX);
    if (r != rfnm_api_failcode::RFNM_API_OK)
        logger->error("RFNM Error %d", r);

    logger->debug("Set RFNM samplerate to " + std::to_string(current_samplerate));

    rfnm_dev_obj->set_stream_format(rfnm::STREAM_FORMAT_CF32, &buffer_size);
    if (buffer_size <= 0)
        logger->error("RFNM Error (Buffer Size) %d", buffer_size);
    logger->info("Buffer Size is %d\n", buffer_size);

    rfnm_stream = rfnm_dev_obj->rx_stream_create(channel);

    rfnm_stream->start();

    // for (int i = 0; i < rfnm::MIN_RX_BUFCNT; i++)
    // {
    //     rx_buffers[i].buf = dsp::create_volk_buffer<uint8_t>(buffer_size);
    //     rfnm_dev_obj->rx_qbuf(&rx_buffers[i]);
    // }

    is_started = true;

    set_frequency(d_frequency);

    set_gains();
    set_others();

    thread_should_run = true;
    work_thread = std::thread(&RFNMSource::mainThread, this);
}

void RFNMSource::stop()
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
        //  rfnm_dev_obj->set_rx_channel_active(channel, RFNM_CH_OFF, RFNM_CH_STREAM_OFF);
        rfnm_stream->stop();
        rfnm_dev_obj->set(channel == 1 ? rfnm::APPLY_CH1_RX : rfnm::APPLY_CH0_RX);
        delete rfnm_stream;
        delete rfnm_dev_obj;

        //  for (int i = 0; i < rfnm::MIN_RX_BUFCNT; i++)
        //      volk_free(rx_buffers[i].buf);
    }
    is_started = false;
}

void RFNMSource::close()
{
}

void RFNMSource::set_frequency(uint64_t frequency)
{
    if (is_started)
    {
        rfnm_dev_obj->set_rx_channel_freq(channel, frequency);
        rfnm_api_failcode r = rfnm_dev_obj->set(channel == 1 ? rfnm::APPLY_CH1_RX : rfnm::APPLY_CH0_RX);
        if (r != rfnm_api_failcode::RFNM_API_OK)
            logger->error("RFNM Error %d", r);
        logger->debug("Set RFNM frequency to %llu", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void RFNMSource::drawControlUI()
{
    if (is_started)
        RImGui::beginDisabled();

    samplerate_widget.render();

    if (RImGui::RadioButton("Channel 0", channel == 0))
        channel = 0;
    RImGui::SameLine();
    if (RImGui::RadioButton("Channel 1", channel == 1))
        channel = 1;

    if (is_started)
        RImGui::endDisabled();

    if (bandwidth_widget.render())
        set_others();

    // Gain settings
    bool gain_changed = false;

    gain_changed |= RImGui::SteppedSliderInt("Gain", &gain, min_gain, max_gain);

    if (gain_changed)
        set_gains();

    if (RImGui::Checkbox("FM Notch", &fmnotch_enabled))
        set_others();
    RImGui::SameLine();
    if (RImGui::Checkbox("Bias", &bias_enabled))
        set_others();

    double vm = corr_mag;
    RImGui::InputDouble("Mag", &vm);
    corr_mag = vm;

    vm = corr_phase;
    RImGui::InputDouble("Pha", &vm);
    corr_phase = vm;
}

void RFNMSource::set_samplerate(uint64_t samplerate)
{
    if (!samplerate_widget.set_value(samplerate, 10e6))
        throw satdump_exception("Unsupported samplerate : " + std::to_string(samplerate) + "!");
}

uint64_t RFNMSource::get_samplerate()
{
    return samplerate_widget.get_value();
}

std::vector<dsp::SourceDescriptor> RFNMSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    auto devlist = rfnm::device::find(rfnm::TRANSPORT_USB);

    for (auto &dev : devlist)
    {
        results.push_back({"rfnm",
                           "RFNM " + std::string(dev.daughterboard->user_readable_name) + " " + std::string(dev.motherboard.user_readable_name) + " " + std::string((char *)dev.motherboard.serial_number),
                           std::string((char *)dev.motherboard.serial_number)});
    }

    return results;
}

#include "sdrplay_sdr.h"
#include <thread>

namespace sdrplay_settings
{
    sdrplay_api_AgcControlT agc_modes[] = {
        sdrplay_api_AGC_DISABLE,
        sdrplay_api_AGC_5HZ,
        sdrplay_api_AGC_50HZ,
        sdrplay_api_AGC_100HZ};
};

void SDRPlaySource::event_callback(sdrplay_api_EventT, sdrplay_api_TunerSelectT, sdrplay_api_EventParamsT *, void *)
{
}

void SDRPlaySource::stream_callback(short *real, short *imag, sdrplay_api_StreamCbParamsT *, unsigned int cnt, unsigned int, void *ctx)
{
    std::shared_ptr<dsp::stream<complex_t>> stream = *((std::shared_ptr<dsp::stream<complex_t>> *)ctx);
    for (unsigned int i = 0; i < cnt; i++)
        stream->writeBuf[i] = complex_t(real[i] / 32768.0f, imag[i] / 32768.0f);
    stream->swap(cnt);
}

void SDRPlaySource::set_gains()
{
    if (!is_started)
        return;

    // Gains
    channel_params->tunerParams.gain.LNAstate = (max_gain - 1) - lna_gain;
    channel_params->tunerParams.gain.gRdB = 59 - (if_gain - 20);
    sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_Tuner_Gr, sdrplay_api_Update_Ext1_None);
    logger->debug("Set SDRPlay LNA to %d", channel_params->tunerParams.gain.LNAstate);
    logger->debug("Set SDRPlay IF gain to %d", channel_params->tunerParams.gain.gRdB);
}

void SDRPlaySource::set_bias()
{
    if (!is_started)
        return;

    if (sdrplay_dev.hwVer == SDRPLAY_RSP1A_ID || sdrplay_dev.hwVer == SDRPLAY_RSP1B_ID) // RSP1A or RSP1B
    {
        channel_params->rsp1aTunerParams.biasTEnable = bias;
        sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_Rsp1a_BiasTControl, sdrplay_api_Update_Ext1_None);
        logger->debug("Set SDRPlay bias to %d", (int)bias);
    }
    else if (sdrplay_dev.hwVer == SDRPLAY_RSP2_ID) // RSP2
    {
        channel_params->rsp2TunerParams.biasTEnable = bias;
        sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_Rsp2_BiasTControl, sdrplay_api_Update_Ext1_None);
        logger->debug("Set SDRPlay bias to %d", (int)bias);
    }
    else if (sdrplay_dev.hwVer == SDRPLAY_RSPdx_ID) // RSPdx
    {
        dev_params->devParams->rspDxParams.biasTEnable = bias;
        sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_None, sdrplay_api_Update_RspDx_BiasTControl);
        logger->debug("Set SDRPlay bias to %d", (int)bias);
    }
    else if (sdrplay_dev.hwVer == SDRPLAY_RSPduo_ID) // RSPduo
    {
        channel_params->rspDuoTunerParams.biasTEnable = bias;
        sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_RspDuo_BiasTControl, sdrplay_api_Update_Ext1_None);
        logger->debug("Set SDRPlay bias to %d", (int)bias);
    }
}

void SDRPlaySource::set_duo_channel()
{
    if (!is_started)
        channel_params = antenna_input == 2 ? dev_params->rxChannelB : dev_params->rxChannelA;
}

void SDRPlaySource::set_duo_tuner()
{
    if (!is_started)
        sdrplay_dev.tuner = antenna_input == 2 ? sdrplay_api_Tuner_B : sdrplay_api_Tuner_A;
}

void SDRPlaySource::set_others()
{
    if (!is_started)
        return;

    if (sdrplay_dev.hwVer == SDRPLAY_RSP1A_ID || sdrplay_dev.hwVer == SDRPLAY_RSP1B_ID) // RSP1A or RSP1B
    {
        dev_params->devParams->rsp1aParams.rfNotchEnable = fm_notch;
        dev_params->devParams->rsp1aParams.rfDabNotchEnable = dab_notch;
        sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_Rsp1a_RfNotchControl, sdrplay_api_Update_Ext1_None);
        sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_Rsp1a_RfDabNotchControl, sdrplay_api_Update_Ext1_None);
        logger->debug("Set SDRPlay FM Notch to %d", (int)fm_notch);
        logger->debug("Set SDRPlay DAB Notch to %d", (int)dab_notch);
    }
    else if (sdrplay_dev.hwVer == SDRPLAY_RSP2_ID) // RSP2
    {
        channel_params->rsp2TunerParams.rfNotchEnable = fm_notch;
        channel_params->rsp2TunerParams.antennaSel = antenna_input == 2 ? sdrplay_api_Rsp2_ANTENNA_B : sdrplay_api_Rsp2_ANTENNA_A;
        channel_params->rsp2TunerParams.amPortSel = antenna_input == 0 ? sdrplay_api_Rsp2_AMPORT_2 : sdrplay_api_Rsp2_AMPORT_1;
        sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_Rsp2_RfNotchControl, sdrplay_api_Update_Ext1_None);
        sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_Rsp2_AntennaControl, sdrplay_api_Update_Ext1_None);
        sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_Rsp2_AmPortSelect, sdrplay_api_Update_Ext1_None);
        logger->debug("Set SDRPlay FM Notch to %d", (int)fm_notch);
        logger->debug("Set SDRPlay Antenna to %d", antenna_input);
    }
    else if (sdrplay_dev.hwVer == SDRPLAY_RSPduo_ID) // RSPDuo
    {
        channel_params->rspDuoTunerParams.rfNotchEnable = fm_notch;
        channel_params->rspDuoTunerParams.rfDabNotchEnable = dab_notch;
        channel_params->rspDuoTunerParams.tuner1AmNotchEnable = am_notch;
        channel_params->rspDuoTunerParams.tuner1AmPortSel = antenna_input == 0 ? sdrplay_api_RspDuo_AMPORT_2 : sdrplay_api_RspDuo_AMPORT_1;
        sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_RspDuo_RfNotchControl, sdrplay_api_Update_Ext1_None);
        sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_RspDuo_Tuner1AmNotchControl, sdrplay_api_Update_Ext1_None);
        sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_RspDuo_RfDabNotchControl, sdrplay_api_Update_Ext1_None);
        sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_RspDuo_AmPortSelect, sdrplay_api_Update_Ext1_None);
        logger->debug("Set SDRPlay FM Notch to %d", (int)fm_notch);
        logger->debug("Set SDRPlay AM Notch to %d", (int)am_notch);
        logger->debug("Set SDRPlay DAB Notch to %d", (int)dab_notch);
        logger->debug("Set SDRPlay Antenna to %d", antenna_input);
    }
    else if (sdrplay_dev.hwVer == SDRPLAY_RSPdx_ID) // RSPdx
    {
        dev_params->devParams->rspDxParams.rfNotchEnable = fm_notch;
        dev_params->devParams->rspDxParams.rfDabNotchEnable = dab_notch;
        dev_params->devParams->rspDxParams.antennaSel = antenna_input == 2 ? sdrplay_api_RspDx_ANTENNA_C : (antenna_input == 1 ? sdrplay_api_RspDx_ANTENNA_B : sdrplay_api_RspDx_ANTENNA_A);
        sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_None, sdrplay_api_Update_RspDx_RfNotchControl);
        sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_None, sdrplay_api_Update_RspDx_AntennaControl);
        sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_None, sdrplay_api_Update_RspDx_RfDabNotchControl);
        logger->debug("Set SDRPlay FM Notch to %d", (int)fm_notch);
        logger->debug("Set SDRPlay DAB Notch to %d", (int)dab_notch);
        logger->debug("Set SDRPlay Antenna to %d", antenna_input);
    }
}

void SDRPlaySource::set_agcs()
{
    // AGC. No idea if those settings are perfect, probably not
    channel_params->ctrlParams.agc.enable = sdrplay_settings::agc_modes[agc_mode];
    channel_params->ctrlParams.agc.attack_ms = 600;
    channel_params->ctrlParams.agc.decay_ms = 600;
    channel_params->ctrlParams.agc.decay_delay_ms = 100;
    channel_params->ctrlParams.agc.decay_threshold_dB = 5;
    channel_params->ctrlParams.agc.setPoint_dBfs = -30;
    sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_Ctrl_Agc, sdrplay_api_Update_Ext1_None);
}

void SDRPlaySource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    lna_gain = getValueOrDefault(d_settings["lna_gain"], lna_gain);
    if_gain = getValueOrDefault(d_settings["if_gain"], if_gain);
    bias = getValueOrDefault(d_settings["bias"], bias);
    fm_notch = getValueOrDefault(d_settings["fm_notch"], fm_notch);
    dab_notch = getValueOrDefault(d_settings["dab_notch"], dab_notch);
    am_notch = getValueOrDefault(d_settings["am_notch"], am_notch);
    antenna_input = getValueOrDefault(d_settings["antenna_input"], antenna_input);
    agc_mode = getValueOrDefault(d_settings["agc_mode"], agc_mode);

    if (is_open && is_started)
    {
        if (sdrplay_dev.hwVer == SDRPLAY_RSPduo_ID)
            set_duo_channel();

        set_gains();
        set_bias();
        set_agcs();
        set_others();
    }
}

nlohmann::json SDRPlaySource::get_settings()
{
    d_settings["lna_gain"] = lna_gain;
    d_settings["if_gain"] = if_gain;
    d_settings["bias"] = bias;
    d_settings["fm_notch"] = fm_notch;
    d_settings["dab_notch"] = dab_notch;
    d_settings["am_notch"] = am_notch;
    d_settings["antenna_input"] = antenna_input;
    d_settings["agc_mode"] = agc_mode;

    return d_settings;
}

void SDRPlaySource::open()
{
    sdrplay_dev = devices_addresses[d_sdr_id];

    sdrplay_dev.tuner = sdrplay_api_Tuner_A;
    sdrplay_dev.rspDuoMode = sdrplay_api_RspDuoMode_Single_Tuner;

    if (sdrplay_api_SelectDevice(&sdrplay_dev) != sdrplay_api_Success)
        logger->critical("Could not open SDRPlay device!");
    logger->info("Opened SDRPlay device!");

    is_open = true;

    // Set available samplerates
    std::vector<double> available_samplerates;
    available_samplerates.push_back(2000000);
    available_samplerates.push_back(3000000);
    available_samplerates.push_back(4000000);
    available_samplerates.push_back(5000000);
    available_samplerates.push_back(6000000);
    available_samplerates.push_back(7000000);
    available_samplerates.push_back(8000000);
    available_samplerates.push_back(9000000);
    available_samplerates.push_back(10000000);

    samplerate_widget.set_list(available_samplerates, false);

    // Set max gain
    if (sdrplay_dev.hwVer == SDRPLAY_RSP1_ID) // RSP1
        max_gain = 4;
    else if (sdrplay_dev.hwVer == SDRPLAY_RSP1A_ID || sdrplay_dev.hwVer == SDRPLAY_RSP1B_ID) // RSP1A or RSP1B
        max_gain = 10;
    else if (sdrplay_dev.hwVer == SDRPLAY_RSP2_ID) // RSP2
        max_gain = 9;
    else if (sdrplay_dev.hwVer == SDRPLAY_RSPduo_ID) // RSPDuo
        max_gain = 10;
    else if (sdrplay_dev.hwVer == SDRPLAY_RSPdx_ID) // RSPdx
        max_gain = 28;
    sdrplay_api_ReleaseDevice(&sdrplay_dev);
}

void SDRPlaySource::start()
{
    DSPSampleSource::start();

    uint64_t current_samplerate = samplerate_widget.get_value();

    if (sdrplay_dev.hwVer == SDRPLAY_RSPduo_ID)
        set_duo_tuner();
    if (sdrplay_api_SelectDevice(&sdrplay_dev) != sdrplay_api_Success)
        logger->critical("Could not open SDRPlay device!");
    logger->info("Opened SDRPlay device!");

    // Prepare device
    sdrplay_api_UnlockDeviceApi();
    sdrplay_api_DebugEnable(sdrplay_dev.dev, sdrplay_api_DbgLvl_Message);

    // Callbacks
    callback_funcs.EventCbFn = event_callback;
    callback_funcs.StreamACbFn = stream_callback;
    callback_funcs.StreamBCbFn = stream_callback;

    if (sdrplay_api_GetDeviceParams(sdrplay_dev.dev, &dev_params) != sdrplay_api_Success)
        throw std::runtime_error("Error getting SDRPlay device params!");

    // Get channel params
    channel_params = dev_params->rxChannelA;
    if (sdrplay_dev.hwVer == SDRPLAY_RSPduo_ID)
        set_duo_channel();

    if (sdrplay_api_Init(sdrplay_dev.dev, &callback_funcs, &output_stream) != sdrplay_api_Success)
        throw std::runtime_error("Error starting SDRPlay device!");

    // Initial freq
    channel_params->tunerParams.rfFreq.rfHz = d_frequency;
    sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_Tuner_Frf, sdrplay_api_Update_Ext1_None);

    logger->debug("Set SDRPlay samplerate to " + std::to_string(current_samplerate));
    dev_params->devParams->fsFreq.fsHz = current_samplerate;

    // Select right bandwidth
    if (current_samplerate <= 200e3)
        channel_params->tunerParams.bwType = sdrplay_api_BW_0_200;
    else if (current_samplerate <= 300e3)
        channel_params->tunerParams.bwType = sdrplay_api_BW_0_300;
    else if (current_samplerate <= 600e3)
        channel_params->tunerParams.bwType = sdrplay_api_BW_0_600;
    else if (current_samplerate <= 1536e3)
        channel_params->tunerParams.bwType = sdrplay_api_BW_1_536;
    else if (current_samplerate <= 5e6)
        channel_params->tunerParams.bwType = sdrplay_api_BW_5_000;
    else if (current_samplerate <= 6e6)
        channel_params->tunerParams.bwType = sdrplay_api_BW_6_000;
    else if (current_samplerate <= 7e6)
        channel_params->tunerParams.bwType = sdrplay_api_BW_7_000;
    else // Otherwise default to max for >= 8MSPS
        channel_params->tunerParams.bwType = sdrplay_api_BW_8_000;

    // Update samplerate & filter BW
    sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_Dev_Fs, sdrplay_api_Update_Ext1_None);
    sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_Tuner_BwType, sdrplay_api_Update_Ext1_None);

    // Sampling settings
    channel_params->ctrlParams.decimation.enable = false;
    channel_params->ctrlParams.dcOffset.DCenable = true;
    channel_params->ctrlParams.dcOffset.IQenable = true;
    sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_Ctrl_Decimation, sdrplay_api_Update_Ext1_None);
    sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_Ctrl_DCoffsetIQimbalance, sdrplay_api_Update_Ext1_None);

    // IF and LO config
    channel_params->tunerParams.ifType = sdrplay_api_IF_Zero;
    channel_params->tunerParams.loMode = sdrplay_api_LO_Auto;
    sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_Tuner_IfType, sdrplay_api_Update_Ext1_None);
    sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_Tuner_LoMode, sdrplay_api_Update_Ext1_None);

    is_started = true;

    set_gains();
    set_bias();
    set_agcs();
    set_others();
}

void SDRPlaySource::stop()
{
    if (is_started)
    {
        output_stream->stopWriter();
        sdrplay_api_Uninit(sdrplay_dev.dev);
        sdrplay_api_ReleaseDevice(&sdrplay_dev);
    }
    is_started = false;
}

void SDRPlaySource::close()
{
    is_open = false;
}

void SDRPlaySource::set_frequency(uint64_t frequency)
{
    if (is_started)
    {
        channel_params->tunerParams.rfFreq.rfHz = frequency;
        sdrplay_api_Update(sdrplay_dev.dev, sdrplay_dev.tuner, sdrplay_api_Update_Tuner_Frf, sdrplay_api_Update_Ext1_None);
        logger->debug("Set SDRPlay frequency to %d", d_frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void SDRPlaySource::drawControlUI()
{
    if (is_started)
        RImGui::beginDisabled();

    samplerate_widget.render();

    if (is_started)
        RImGui::endDisabled();

    if (!is_started)
        RImGui::beginDisabled();
    // Gain settings
    bool gain_changed = false;
    gain_changed |= RImGui::SteppedSliderInt("LNA Gain", &lna_gain, 0, max_gain - 1);
    gain_changed |= RImGui::SteppedSliderInt("IF Gain", &if_gain, 20, 59);
    if (gain_changed)
        set_gains();

    if (RImGui::Combo("AGC Mode", &agc_mode, "OFF\0"
                                             "5HZ\0"
                                             "50HZ\0"
                                             "100HZ\0"))
        set_agcs();

    // RSP1A-specific settings
    if (sdrplay_dev.hwVer == SDRPLAY_RSP1A_ID || sdrplay_dev.hwVer == SDRPLAY_RSP1B_ID)
    {
        if (RImGui::Checkbox("FM Notch", &fm_notch))
            set_others();
        if (RImGui::Checkbox("DAB Notch", &dab_notch))
            set_others();
        if (RImGui::Checkbox("Bias", &bias))
            set_bias();
    }
    // RSP2-specific settings
    else if (sdrplay_dev.hwVer == SDRPLAY_RSP2_ID)
    {
        if (RImGui::Combo("Antenna", &antenna_input, "Antenna A\0"
                                                     "Antenna A (Hi-Z)\0"
                                                     "Antenna B\0"))
            set_others();
        if (RImGui::Checkbox("FM Notch", &fm_notch))
            set_others();
        if (RImGui::Checkbox("Bias", &bias))
            set_bias();
    }
    // RSPDuo-specific settings
    else if (sdrplay_dev.hwVer == SDRPLAY_RSPduo_ID)
    {
        if (is_started)
            RImGui::beginDisabled();
        else
            RImGui::endDisabled();
        RImGui::Combo("Antenna", &antenna_input, "Antenna A\0"
                                                 "Antenna A (Hi-Z)\0"
                                                 "Antenna B\0");
        if (is_started)
            RImGui::endDisabled();
        else
            RImGui::beginDisabled();

        if (RImGui::Checkbox("AM Notch", &am_notch))
            set_others();
        if (RImGui::Checkbox("FM Notch", &fm_notch))
            set_others();
        if (RImGui::Checkbox("DAB Notch", &dab_notch))
            set_others();
        if (RImGui::Checkbox("Bias", &bias))
            set_bias();
    }
    // RSPDx-specific settings
    else if (sdrplay_dev.hwVer == SDRPLAY_RSPdx_ID)
    {
        if (RImGui::Combo("Antenna", &antenna_input, "Antenna A\0"
                                                     "Atenna B\0"
                                                     "Atenna C\0"))
            set_others();
        if (RImGui::Checkbox("DAB Notch", &dab_notch))
            set_others();
        if (RImGui::Checkbox("FM Notch", &fm_notch))
            set_others();
        if (RImGui::Checkbox("Bias", &bias))
            set_bias();
    }
    else
    {
        RImGui::Text("%s", "This device is not supported yet,\n or perhaps a clone!");
    }
    if (!is_started)
        RImGui::endDisabled();
}

void SDRPlaySource::set_samplerate(uint64_t samplerate)
{
    if (!samplerate_widget.set_value(samplerate, 10e6))
        throw std::runtime_error("Unspported samplerate : " + std::to_string(samplerate) + "!");
}

uint64_t SDRPlaySource::get_samplerate()
{
    return samplerate_widget.get_value();
}

std::vector<dsp::SourceDescriptor> SDRPlaySource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    unsigned int c = 0;
    sdrplay_api_GetDevices(devices_addresses, &c, 128);

    for (unsigned int i = 0; i < c; i++)
    {
        std::stringstream ss;
        uint64_t id = 0;
        ss << devices_addresses[i].SerNo;
        ss >> std::hex >> id;
        ss << devices_addresses[i].SerNo;

        // Handle all versions
        if (devices_addresses[i].hwVer == SDRPLAY_RSP1_ID)
            results.push_back({"sdrplay", "RSP1 " + ss.str(), i});
        else if (devices_addresses[i].hwVer == SDRPLAY_RSP1A_ID)
            results.push_back({"sdrplay", "RSP1A " + ss.str(), i});
        else if (devices_addresses[i].hwVer == SDRPLAY_RSP1B_ID)
            results.push_back({"sdrplay", "RSP1B " + ss.str(), i});
        else if (devices_addresses[i].hwVer == SDRPLAY_RSP2_ID)
            results.push_back({"sdrplay", "RSP2 " + ss.str(), i});
        else if (devices_addresses[i].hwVer == SDRPLAY_RSPduo_ID)
            results.push_back({"sdrplay", "RSPDuo " + ss.str(), i});
        else if (devices_addresses[i].hwVer == SDRPLAY_RSPdx_ID)
            results.push_back({"sdrplay", "RSPdx " + ss.str(), i});
        else
            results.push_back({"sdrplay", "Unknown RSP " + ss.str(), i});
    }

    return results;
}

sdrplay_api_DeviceT SDRPlaySource::devices_addresses[128];

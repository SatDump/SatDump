#include "sdrplay.h"

#ifndef DISABLE_SDR_SDRPLAY
#include <sstream>
#include "imgui/imgui.h"
#include "logger.h"

void SDRPlay::event_callback(sdrplay_api_EventT /*id*/, sdrplay_api_TunerSelectT /*tuner*/, sdrplay_api_EventParamsT * /*params*/, void * /*ctx*/)
{
}

void SDRPlay::stream_callback(short *real, short *imag, sdrplay_api_StreamCbParamsT * /*params*/, unsigned int cnt, unsigned int /*reset*/, void *ctx)
{
    if (cnt <= 0)
        return;

    std::shared_ptr<dsp::stream<complex_t>> stream = *((std::shared_ptr<dsp::stream<complex_t>> *)ctx);
    // Convert to CF-32
    for (unsigned int i = 0; i < cnt; i++)
        stream->writeBuf[i] = complex_t(real[i] / 32768.0f, imag[i] / 32768.0f);
    stream->swap(cnt);
}

namespace sdrplay_settings
{
    sdrplay_api_AgcControlT agc_modes[] = {
        sdrplay_api_AGC_DISABLE,
        sdrplay_api_AGC_5HZ,
        sdrplay_api_AGC_50HZ,
        sdrplay_api_AGC_100HZ};
};

SDRPlay::SDRPlay(std::map<std::string, std::string> parameters, uint64_t id) : SDRDevice(parameters, id)
{
    READ_PARAMETER_IF_EXISTS_FLOAT(lna_gain, "lna_gain");
    READ_PARAMETER_IF_EXISTS_FLOAT(if_gain, "if_gain");
    READ_PARAMETER_IF_EXISTS_FLOAT(bias, "bias");
    READ_PARAMETER_IF_EXISTS_FLOAT(agc_mode, "agc_mode");
    READ_PARAMETER_IF_EXISTS_FLOAT(fm_notch, "fm_notch");
    READ_PARAMETER_IF_EXISTS_FLOAT(dab_notch, "dab_notch");
    READ_PARAMETER_IF_EXISTS_FLOAT(am_notch, "am_notch");
    READ_PARAMETER_IF_EXISTS_FLOAT(antenna_input, "antenna_input");
    READ_PARAMETER_IF_EXISTS_FLOAT(am_port, "am_port");

    dev = devices_addresses[id];

    dev.tuner = sdrplay_api_Tuner_A;
    dev.rspDuoMode = sdrplay_api_RspDuoMode_Single_Tuner;

    if (sdrplay_api_SelectDevice(&dev) != sdrplay_api_Success)
    {
        logger->critical("Could not open SDRPlay device!");
        return;
    }
    logger->info("Opened SDRPlay device!");

    std::fill(frequency, &frequency[100], 0);
}

std::map<std::string, std::string> SDRPlay::getParameters()
{
    d_parameters["lna_gain"] = std::to_string(lna_gain);
    d_parameters["if_gain"] = std::to_string(if_gain);
    d_parameters["bias"] = std::to_string(bias);
    d_parameters["agc_mode"] = std::to_string(agc_mode);
    d_parameters["fm_notch"] = std::to_string(fm_notch);
    d_parameters["dab_notch"] = std::to_string(dab_notch);
    d_parameters["am_notch"] = std::to_string(am_notch);
    d_parameters["antenna_input"] = std::to_string(antenna_input);
    d_parameters["am_port"] = std::to_string(am_port);

    return d_parameters;
}

void SDRPlay::start()
{
    sdrplay_api_UnlockDeviceApi();
    sdrplay_api_DebugEnable(dev.dev, sdrplay_api_DbgLvl_Message);

    // Callbacks
    callback_funcs.EventCbFn = event_callback;
    callback_funcs.StreamACbFn = stream_callback;
    callback_funcs.StreamBCbFn = stream_callback;

    if (sdrplay_api_GetDeviceParams(dev.dev, &dev_params) != sdrplay_api_Success)
    {
        logger->info("Error getting device params!");
        return;
    }

    if (sdrplay_api_Init(dev.dev, &callback_funcs, &output_stream) != sdrplay_api_Success)
    {
        logger->info("Error starting SDRPlay device!");
        return;
    }

    // Get channel
    channel_params = dev_params->rxChannelA;

    // Set per-device settings such as max gain, and custom params
    if (dev.hwVer == SDRPLAY_RSP1_ID) // RSP1
    {
        max_gain = 4;
    }
    else if (dev.hwVer == SDRPLAY_RSP1A_ID) // RSP1A
    {
        // Set
        dev_params->devParams->rsp1aParams.rfNotchEnable = fm_notch;
        dev_params->devParams->rsp1aParams.rfDabNotchEnable = dab_notch;
        channel_params->rsp1aTunerParams.biasTEnable = bias;

        // Apply
        sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Rsp1a_RfNotchControl, sdrplay_api_Update_Ext1_None);
        sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Rsp1a_RfDabNotchControl, sdrplay_api_Update_Ext1_None);
        sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Rsp1a_BiasTControl, sdrplay_api_Update_Ext1_None);

        max_gain = 10;
    }
    else if (dev.hwVer == SDRPLAY_RSP2_ID) // RSP2
    {
        // Set
        channel_params->rsp2TunerParams.rfNotchEnable = fm_notch;
        channel_params->rsp2TunerParams.biasTEnable = bias;
        channel_params->rsp2TunerParams.antennaSel = antenna_input == 1 ? sdrplay_api_Rsp2_ANTENNA_B : sdrplay_api_Rsp2_ANTENNA_A;
        channel_params->rsp2TunerParams.amPortSel = am_port == 1 ? sdrplay_api_Rsp2_AMPORT_2 : sdrplay_api_Rsp2_AMPORT_1;

        // Apply
        sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Rsp2_RfNotchControl, sdrplay_api_Update_Ext1_None);
        sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Rsp2_AntennaControl, sdrplay_api_Update_Ext1_None);
        sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Rsp2_AmPortSelect, sdrplay_api_Update_Ext1_None);
        sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Rsp2_BiasTControl, sdrplay_api_Update_Ext1_None);

        max_gain = 9;
    }
    else if (dev.hwVer == SDRPLAY_RSPdx_ID) // RSPdx
    {
        // Set
        dev_params->devParams->rspDxParams.rfNotchEnable = fm_notch;
        dev_params->devParams->rspDxParams.rfDabNotchEnable = dab_notch;
        dev_params->devParams->rspDxParams.biasTEnable = bias;
        dev_params->devParams->rspDxParams.antennaSel = antenna_input == 2 ? sdrplay_api_RspDx_ANTENNA_C : (antenna_input == 1 ? sdrplay_api_RspDx_ANTENNA_B : sdrplay_api_RspDx_ANTENNA_A);

        // Apply
        sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_None, sdrplay_api_Update_RspDx_RfNotchControl);
        sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_None, sdrplay_api_Update_RspDx_AntennaControl);
        sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_None, sdrplay_api_Update_RspDx_BiasTControl);
        sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_None, sdrplay_api_Update_RspDx_RfDabNotchControl);

        max_gain = 28;
    }

    // Frequency
    logger->info("Frequency " + std::to_string(d_frequency));
    channel_params->tunerParams.rfFreq.rfHz = d_frequency;
    sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Tuner_Frf, sdrplay_api_Update_Ext1_None);

    // Samplerate
    logger->info("Samplerate " + std::to_string(d_samplerate));
    dev_params->devParams->fsFreq.fsHz = d_samplerate;

    // Select right bandwidth
    if (d_samplerate <= 200e3)
        channel_params->tunerParams.bwType = sdrplay_api_BW_0_200;
    else if (d_samplerate <= 300e3)
        channel_params->tunerParams.bwType = sdrplay_api_BW_0_300;
    else if (d_samplerate <= 600e3)
        channel_params->tunerParams.bwType = sdrplay_api_BW_0_600;
    else if (d_samplerate <= 1536e3)
        channel_params->tunerParams.bwType = sdrplay_api_BW_1_536;
    else if (d_samplerate <= 5e6)
        channel_params->tunerParams.bwType = sdrplay_api_BW_5_000;
    else if (d_samplerate <= 6e6)
        channel_params->tunerParams.bwType = sdrplay_api_BW_6_000;
    else if (d_samplerate <= 7e6)
        channel_params->tunerParams.bwType = sdrplay_api_BW_7_000;
    else // Otherwise default to max for >= 8MSPS
        channel_params->tunerParams.bwType = sdrplay_api_BW_8_000;

    sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Dev_Fs, sdrplay_api_Update_Ext1_None);
    sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Tuner_BwType, sdrplay_api_Update_Ext1_None);

    // Sampling settings
    channel_params->ctrlParams.decimation.enable = false;
    channel_params->ctrlParams.dcOffset.DCenable = true;
    channel_params->ctrlParams.dcOffset.IQenable = true;
    sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Ctrl_Decimation, sdrplay_api_Update_Ext1_None);
    sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Ctrl_DCoffsetIQimbalance, sdrplay_api_Update_Ext1_None);

    // IF and LO config
    channel_params->tunerParams.ifType = sdrplay_api_IF_Zero;
    channel_params->tunerParams.loMode = sdrplay_api_LO_Auto;
    sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Tuner_IfType, sdrplay_api_Update_Ext1_None);
    sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Tuner_LoMode, sdrplay_api_Update_Ext1_None);

    // AGC. No idea if those settings are perfect, probably not
    channel_params->ctrlParams.agc.enable = sdrplay_settings::agc_modes[agc_mode];
    channel_params->ctrlParams.agc.attack_ms = 600;
    channel_params->ctrlParams.agc.decay_ms = 600;
    channel_params->ctrlParams.agc.decay_delay_ms = 100;
    channel_params->ctrlParams.agc.decay_threshold_dB = 5;
    channel_params->ctrlParams.agc.setPoint_dBfs = -30;
    sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Ctrl_Agc, sdrplay_api_Update_Ext1_None);

    // Gains
    channel_params->tunerParams.gain.LNAstate = (max_gain - 1) - lna_gain;
    channel_params->tunerParams.gain.gRdB = (59 - 1) - if_gain;
    sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Tuner_Gr, sdrplay_api_Update_Ext1_None);
}

void SDRPlay::stop()
{
    sdrplay_api_Uninit(dev.dev);
    sdrplay_api_ReleaseDevice(&dev);
}

SDRPlay::~SDRPlay()
{
}

void SDRPlay::drawUI()
{
    ImGui::Begin("SDRPlay Control", NULL);

    // Settings common to all RSPs
    {
        ImGui::InputText("MHz", frequency, 100);
        ImGui::SameLine();
        if (ImGui::Button("Set"))
        {
            d_frequency = std::stof(frequency) * 1e6;
            channel_params->tunerParams.rfFreq.rfHz = d_frequency;
            sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Tuner_Frf, sdrplay_api_Update_Ext1_None);
        }
        if (ImGui::SliderInt("LNA Gain", &lna_gain, 0, max_gain))
        {
            channel_params->tunerParams.gain.LNAstate = (max_gain - 1) - lna_gain;
            sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Tuner_Gr, sdrplay_api_Update_Ext1_None);
        }
        if (ImGui::SliderInt("IF Gain", &if_gain, 20, 59))
        {
            channel_params->tunerParams.gain.gRdB = (59 - 1) - if_gain;
            sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Tuner_Gr, sdrplay_api_Update_Ext1_None);
        }
        if (ImGui::Combo("AGC Mode", &agc_mode, "OFF\0"
                                                "5HZ\0"
                                                "50HZ\0"
                                                "100HZ\0"))
        {
            channel_params->ctrlParams.agc.enable = sdrplay_settings::agc_modes[agc_mode];
            sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Ctrl_Agc, sdrplay_api_Update_Ext1_None);
        }
    }

    // RSP1A-specific settings
    if (dev.hwVer == SDRPLAY_RSP1A_ID)
    {
        if (ImGui::Checkbox("FM Notch", &fm_notch))
        {
            dev_params->devParams->rsp1aParams.rfNotchEnable = fm_notch;
            sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Rsp1a_RfNotchControl, sdrplay_api_Update_Ext1_None);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("DAB Notch", &dab_notch))
        {
            dev_params->devParams->rsp1aParams.rfDabNotchEnable = dab_notch;
            sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Rsp1a_RfDabNotchControl, sdrplay_api_Update_Ext1_None);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Bias", &bias))
        {
            channel_params->rsp1aTunerParams.biasTEnable = bias;
            sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Rsp1a_BiasTControl, sdrplay_api_Update_Ext1_None);
        }
    }
    // RSP2-specific settings
    else if (dev.hwVer == SDRPLAY_RSP2_ID)
    {
        if (ImGui::Combo("Antenna", &antenna_input, "Antenna A\0"
                                                    "Atenna B\0"))
        {
            channel_params->rsp2TunerParams.antennaSel = antenna_input == 1 ? sdrplay_api_Rsp2_ANTENNA_B : sdrplay_api_Rsp2_ANTENNA_A;
            sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Rsp2_AntennaControl, sdrplay_api_Update_Ext1_None);
        }
        if (ImGui::Combo("AM Port", &antenna_input, "Port 1\0"
                                                    "Port 2\0"))
        {
            channel_params->rsp2TunerParams.amPortSel = am_port == 1 ? sdrplay_api_Rsp2_AMPORT_2 : sdrplay_api_Rsp2_AMPORT_1;
            sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Rsp2_AmPortSelect, sdrplay_api_Update_Ext1_None);
        }
        if (ImGui::Checkbox("FM Notch", &fm_notch))
        {
            channel_params->rsp2TunerParams.rfNotchEnable = fm_notch;
            sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Rsp2_RfNotchControl, sdrplay_api_Update_Ext1_None);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Bias", &bias))
        {
            channel_params->rsp2TunerParams.biasTEnable = bias;
            sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Rsp2_BiasTControl, sdrplay_api_Update_Ext1_None);
        }
    }
    // RSPDx-specific settings
    else if (dev.hwVer == SDRPLAY_RSPdx_ID)
    {
        if (ImGui::Combo("Antenna", &antenna_input, "Antenna A\0"
                                                    "Atenna B\0"
                                                    "Atenna C\0"))
        {
            dev_params->devParams->rspDxParams.antennaSel = antenna_input == 2 ? sdrplay_api_RspDx_ANTENNA_C : (antenna_input == 1 ? sdrplay_api_RspDx_ANTENNA_B : sdrplay_api_RspDx_ANTENNA_A);
            sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_None, sdrplay_api_Update_RspDx_AntennaControl);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("DAB Notch", &dab_notch))
        {
            dev_params->devParams->rsp1aParams.rfDabNotchEnable = dab_notch;
            sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_None, sdrplay_api_Update_RspDx_RfDabNotchControl);
        }
        if (ImGui::Checkbox("FM Notch", &fm_notch))
        {
            dev_params->devParams->rspDxParams.rfNotchEnable = fm_notch;
            sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_None, sdrplay_api_Update_RspDx_RfNotchControl);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Bias", &bias))
        {
            dev_params->devParams->rspDxParams.biasTEnable = bias;
            sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_None, sdrplay_api_Update_RspDx_BiasTControl);
        }
    }
    else
    {
        ImGui::Text("This device is not supported yet!");
    }

    ImGui::End();
}

void SDRPlay::setFrequency(float frequency)
{
    d_frequency = frequency;
    std::memcpy(this->frequency, std::to_string((float)d_frequency / 1e6).c_str(), std::to_string((float)d_frequency / 1e6).length());
    if (channel_params)
    {
        channel_params->tunerParams.rfFreq.rfHz = d_frequency;
        sdrplay_api_Update(dev.dev, dev.tuner, sdrplay_api_Update_Tuner_Frf, sdrplay_api_Update_Ext1_None);
    }
}

void SDRPlay::init()
{
    sdrplay_api_ErrT error = sdrplay_api_Open();
    if (error != sdrplay_api_Success)
    {
        logger->error("Couuld not open the SDRPlay API, perhaps the service is not running?");
        return;
    }
    logger->info("SDRPlay APi is ready!");
}

std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> SDRPlay::getDevices()
{
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> results;

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
            results.push_back({"RSP1 " + ss.str(), SDRPLAY, i});
        else if (devices_addresses[i].hwVer == SDRPLAY_RSP1A_ID)
            results.push_back({"RSP1A " + ss.str(), SDRPLAY, i});
        else if (devices_addresses[i].hwVer == SDRPLAY_RSP2_ID)
            results.push_back({"RSP2 " + ss.str(), SDRPLAY, i});
        else if (devices_addresses[i].hwVer == SDRPLAY_RSPduo_ID)
            results.push_back({"RSPDuo " + ss.str(), SDRPLAY, i});
        else if (devices_addresses[i].hwVer == SDRPLAY_RSPdx_ID)
            results.push_back({"RSPdx " + ss.str(), SDRPLAY, i});
    }

    return results;
}

std::string SDRPlay::getID()
{
    return "sdrplay";
}

sdrplay_api_DeviceT SDRPlay::devices_addresses[128];
#endif
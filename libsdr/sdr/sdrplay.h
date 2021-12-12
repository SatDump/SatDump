#pragma once

#ifndef DISABLE_SDR_SDRPLAY
#ifdef _WIN32
#include <WinSock2.h>
#endif
#include "sdr.h"
#include <sdrplay_api.h>

class SDRPlay : public SDRDevice
{
private:
    sdrplay_api_DeviceT dev;
    int lna_gain = 0;
    int if_gain = 20;
    bool bias = false;
    bool fm_notch = false;
    bool dab_notch = false;
    bool am_notch = false;
    int am_port = 1;
    int antenna_input = 0;
    int agc_mode = 0;
    char frequency[100];

    sdrplay_api_DeviceParamsT *dev_params = nullptr;
    sdrplay_api_RxChannelParamsT *channel_params = nullptr;
    sdrplay_api_CallbackFnsT callback_funcs;
    static sdrplay_api_DeviceT devices_addresses[128];

    int max_gain;

    static void event_callback(sdrplay_api_EventT id, sdrplay_api_TunerSelectT tuner, sdrplay_api_EventParamsT *params, void *ctx);
    static void stream_callback(short *real, short *imag, sdrplay_api_StreamCbParamsT *params, unsigned int cnt, unsigned int reset, void *ctx);

public:
    SDRPlay(std::map<std::string, std::string> parameters, uint64_t id = 0);
    ~SDRPlay();
    std::map<std::string, std::string> getParameters();
    std::string getID();
    void start();
    void stop();
    void drawUI();
    static void init();
    virtual void setFrequency(float frequency);
    static std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> getDevices();
};
#endif
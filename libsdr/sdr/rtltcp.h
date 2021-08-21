#pragma once

#ifndef DISABLE_SDR_RTLTCP
#include "sdr.h"
#include "rtl_tcp/rtl_tcp_client.h"
#include <thread>

class SDRRtlTcp : public SDRDevice
{
private:
    std::thread workThread;
    void runThread();
    bool should_run;
    RTLTCPClient client;
    bool agc = false;
    int gain = 10;
    char frequency[100];

    uint8_t *samples8;

public:
    SDRRtlTcp(std::map<std::string, std::string> parameters, uint64_t id = 0);
    ~SDRRtlTcp();
    std::map<std::string, std::string> getParameters();
    std::string getID();
    void start();
    void stop();
    void drawUI();
    static void init();
    virtual void setFrequency(float frequency);
    void setGainMode(bool gainmode);
    void setGain(int gain);
    static char server_ip[100];
    static char server_port[100];
    static std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> getDevices();
    static std::map<std::string, std::string> drawParamsUI();
};
#endif
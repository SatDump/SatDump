#pragma once

#include "sdr.h"

#ifndef DISABLE_SDR_SPYSERVER
#include "spyserver/spyserver_client.h"

class SDRSpyServer : public SDRDevice
{
private:
    spyserver::SpyServerClient client;
    int gain = 10;
    int digital_gain = 0;
    int stageToUse = 0;
    SpyServerStreamFormat sampleFormat;
    char frequency[100];

public:
    SDRSpyServer(std::map<std::string, std::string> parameters, uint64_t id = 0);
    ~SDRSpyServer();
    std::map<std::string, std::string> getParameters();
    std::string getID();
    void start();
    void stop();
    void drawUI();
    static void init();
    virtual void setFrequency(float frequency);
    static std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> getDevices();
    static char server_ip[100];
    static char server_port[100];
    static SpyServerStreamFormat serverSampleFormat;
    static std::map<std::string, std::string> drawParamsUI();
};
#endif
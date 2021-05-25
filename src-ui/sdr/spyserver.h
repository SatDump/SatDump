#pragma once

#include "sdr.h"

#ifdef BUILD_LIVE
#include "spyserver/spyserver_client.h"

class SDRSpyServer : public SDRDevice
{
private:
    std::shared_ptr<ss_client> client;
    int gain = 10;
    bool bias = false;
    char frequency[100];

    std::thread workThread;
    void runThread();
    bool should_run;

    int16_t *samples;

public:
    SDRSpyServer(std::map<std::string, std::string> parameters, uint64_t id = 0);
    ~SDRSpyServer();
    void start();
    void stop();
    void drawUI();
    static void init();
    virtual void setFrequency(int frequency);
    static std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> getDevices();
    static char server_ip[100];
    static char server_port[100];
    static std::map<std::string, std::string> drawParamsUI();
};
#endif
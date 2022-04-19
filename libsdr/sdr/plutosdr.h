#pragma once

#ifndef DISABLE_SDR_PLUTOSDR
#include "sdr.h"
#include <thread>

class SDRPlutoSDR : public SDRDevice
{
private:
    void worker();

    std::thread workThread;
    struct iio_context *ctx = NULL;
    struct iio_device *phy = NULL;
    struct iio_device *dev = NULL;
    bool running = false;
    char ip[1024] = "ip:192.168.2.1";
    float gain = 0;

    char frequency[100];

public:
    SDRPlutoSDR(std::map<std::string, std::string> parameters, uint64_t id = 0);
    ~SDRPlutoSDR();
    std::map<std::string, std::string> getParameters();
    std::string getID();
    void start();
    void stop();
    void drawUI();
    virtual void setFrequency(float frequency);
    static char pluto_ip[100];
    void setGain(float gain);
    static std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> getDevices();
    static std::map<std::string, std::string> drawParamsUI();
};
#endif
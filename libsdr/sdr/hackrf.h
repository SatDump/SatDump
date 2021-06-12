#pragma once

#include "sdr.h"
#include <libhackrf/hackrf.h>
#include <thread>

class SDRHackRF : public SDRDevice
{
private:
    struct hackrf_device *dev;
    bool agc = false;
    int lna_gain = 10, vga_gain = 10;
    bool amp = true, bias = false;
    char frequency[100];

    static int _rx_callback(hackrf_transfer *t);

public:
    SDRHackRF(std::map<std::string, std::string> parameters, uint64_t id = 0);
    ~SDRHackRF();
    std::map<std::string, std::string> getParameters();
    std::string getID();
    void start();
    void stop();
    void drawUI();
    static void init();
    virtual void setFrequency(float frequency);
    static std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> getDevices();
};

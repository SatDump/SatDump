#pragma once

#include "sdr.h"

#ifdef BUILD_LIVE
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
    void start();
    void stop();
    void drawUI();
    static void init();
    virtual void setFrequency(int frequency);
    static std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> getDevices();
};
#endif
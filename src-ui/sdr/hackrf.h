#pragma once

#include "sdr.h"

#ifdef BUILD_LIVE
#include <libhackrf/hackrf.h>

class SDRHackRF : public SDRDevice
{
private:
    struct hackrf_device *dev;
    int gain = 10;
    bool bias = false;
    int d_samplerate;
    int d_frequency;

public:
    SDRHackRF();
    void start();
    void stop();
    void drawUI();
    static void init();
    static std::vector<std::string> getDevices();
};
#endif
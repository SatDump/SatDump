#pragma once

#include "modules/buffer.h"
#include "live.h"
#include <vector>

#ifdef BUILD_LIVE
class SDRDevice
{
public:
    std::shared_ptr<dsp::stream<std::complex<float>>> output_stream;
    SDRDevice();
    virtual void start();
    virtual void stop();
    virtual void drawUI();
    static void init();
    static std::vector<std::string> getDevices();
};

void initSDRs();
std::vector<std::string> getAllDevices();
#endif
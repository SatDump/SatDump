#pragma once

#include "sdr.h"

#ifdef BUILD_LIVE
#include <libairspy/airspy.h>

class SDRAirspy : public SDRDevice
{
private:
    struct airspy_device *dev;
    int gain = 10;
    bool bias = false;
    char frequency[100];

    static int _rx_callback(airspy_transfer *t);

public:
    SDRAirspy(std::map<std::string, std::string> parameters, uint64_t id = 0);
    ~SDRAirspy();
    void start();
    void stop();
    void drawUI();
    static void init();
    virtual void setFrequency(long frequency);
    static std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> getDevices();
};
#endif
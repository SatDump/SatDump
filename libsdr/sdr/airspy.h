#pragma once

#ifndef DISABLE_SDR_AIRSPY
#include "sdr.h"
#ifdef __ANDROID__
#include <airspy.h>
#else
#include <libairspy/airspy.h>
#endif

class SDRAirspy : public SDRDevice
{
private:
    struct airspy_device *dev;
    int gain = 10;
    bool bias = false;
    char frequency[100];

    static int _rx_callback(airspy_transfer *t);

public:
#ifdef __ANDROID__
    SDRAirspy(std::map<std::string, std::string> parameters, int fileDescriptor, std::string devicePath);
#endif
    SDRAirspy(std::map<std::string, std::string> parameters, uint64_t id = 0);
    ~SDRAirspy();
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
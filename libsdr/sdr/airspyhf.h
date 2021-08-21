#pragma once

#ifndef DISABLE_SDR_AIRSPYHF
#include "sdr.h"
#ifdef __ANDROID__
#include <airspyhf.h>
#else
#include <libairspyhf/airspyhf.h>
#endif

class SDRAirspyHF : public SDRDevice
{
private:
    struct airspyhf_device *dev;
    int agc_mode = 0;
    int attenuation = 0;
    bool hf_lna = false;
    char frequency[100];

    static int _rx_callback(airspyhf_transfer_t *t);

public:
#ifdef __ANDROID__
    SDRAirspyHF(std::map<std::string, std::string> parameters, int fileDescriptor, std::string devicePath);
#endif
    SDRAirspyHF(std::map<std::string, std::string> parameters, uint64_t id = 0);
    ~SDRAirspyHF();
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
#pragma once

#include "sdr.h"

#ifndef DISABLE_SDR_LIMESDR
#include <lime/lms7_device.h>
#include <lime/Streamer.h>
#include <lime/ConnectionRegistry.h>

class SDRLimeSDR : public SDRDevice
{
private:
    lime::LMS7_Device *limeDevice;
    lime::StreamChannel *limeStream;
    lime::StreamChannel *limeStreamID;
    lime::StreamConfig limeConfig;
    int gain_tia = 10, gain_lna = 10, gain_pga = 10;
    bool bias = false;
    char frequency[100];

    std::thread workThread;
    void runThread();
    bool should_run;

public:
    SDRLimeSDR(std::map<std::string, std::string> parameters, uint64_t id = 0);
    ~SDRLimeSDR();
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
#pragma once

#ifndef DISABLE_SDR_RTLSDR
#include "sdr.h"
#ifdef __ANDROID__
#include <rtl-sdr-android.h>
#endif
#include <rtl-sdr.h>
#include <thread>

class SDRRtlSdr : public SDRDevice
{
private:
    std::thread workThread;
    void runThread();
    bool should_run;
    std::mutex dev_mutex;
    struct rtlsdr_dev *dev;
    bool agc = false;
    int gain = 10;
    bool bias = false;
    char frequency[100];

    static void _rx_callback(unsigned char *buf, uint32_t len, void *ctx);

public:
#ifdef __ANDROID__
    SDRRtlSdr(std::map<std::string, std::string> parameters, int fileDescriptor, std::string devicePath);
#endif
    SDRRtlSdr(std::map<std::string, std::string> parameters, uint64_t id = 0);
    ~SDRRtlSdr();
    std::map<std::string, std::string> getParameters();
    std::string getID();
    void start();
    void stop();
    void drawUI();
    static void init();
    virtual void setFrequency(float frequency);
    void setGainMode(bool gainmode);
    void setGain(int gain);
    void setBias(bool bias);
    static std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> getDevices();
};
#endif
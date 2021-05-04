#pragma once

#include "sdr.h"

#ifdef BUILD_LIVE
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
    int gain = 10;
    bool bias = false;
    char frequency[100];

    static void _rx_callback(unsigned char *buf, uint32_t len, void *ctx);

public:
    SDRRtlSdr(uint64_t id = 0);
    void start();
    void stop();
    void drawUI();
    static void init();
    virtual void setFrequency(int frequency);
    static std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> getDevices();
};
#endif
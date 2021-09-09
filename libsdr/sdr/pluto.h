#pragma once

#include "sdr.h"

#ifndef DISABLE_SDR_PLUTO
#include <thread>
#include <iio.h>

class SDRPluto : public SDRDevice
{
private:
	struct iio_context *ctx;
	struct iio_device *phy;
    struct iio_device *dev;
	struct iio_channel *rx0_i, *rx0_q;
	struct iio_buffer *rxbuf;

    int gain = 10;
    char frequency[100];

    std::thread workThread;
    void runThread();
    bool should_run;

public:
    SDRPluto(std::map<std::string, std::string> parameters, uint64_t id = 0);
    ~SDRPluto();
    std::map<std::string, std::string> getParameters();
    std::string getID();
    void start();
    void stop();
    void drawUI();
    static void init();
    virtual void setFrequency(float frequency);
    static char uri[100];
    static std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> getDevices();
    static std::map<std::string, std::string> drawParamsUI();
};
#endif
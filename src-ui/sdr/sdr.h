#pragma once

#include "modules/buffer.h"
#include "live.h"
#include <vector>

#ifdef BUILD_LIVE
enum sdr_device_type
{
    AIRSPY,
    RTLSDR
};

class SDRDevice
{
protected:
    int d_samplerate;
    int d_frequency;

public:
    std::shared_ptr<dsp::stream<std::complex<float>>> output_stream;
    SDRDevice(uint64_t id = 0);
    virtual void start();
    virtual void stop();
    virtual void drawUI();
    static void init();
    virtual void setFrequency(int frequency);
    virtual void setSamplerate(int samplerate);
    virtual int getFrequency();
    virtual int getSamplerate();
    static std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> getDevices();
};

void initSDRs();
std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> getAllDevices();
std::shared_ptr<SDRDevice> getDeviceByID(std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> devList, int num);
#endif
#pragma once

#include "common/dsp/buffer.h"
#include "live.h"
#include <vector>
#include <map>
#include <string>

#ifdef BUILD_LIVE
enum sdr_device_type
{
    AIRSPY,
    RTLSDR,
    HACKRF,
    LIMESDR,
    SPYSERVER
};

class SDRDevice
{
protected:
    std::map<std::string, std::string> d_parameters;
    float d_samplerate;
    float d_frequency;

public:
    std::shared_ptr<dsp::stream<std::complex<float>>> output_stream;
    SDRDevice(std::map<std::string, std::string> parameters, uint64_t id = 0);
    virtual void start();
    virtual void stop();
    virtual void drawUI();
    static void init();
    virtual void setFrequency(float frequency);
    virtual void setSamplerate(float samplerate);
    virtual int getFrequency();
    virtual int getSamplerate();
    static std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> getDevices();
    static std::map<std::string, std::string> drawParamsUI();
};

void initSDRs();
std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> getAllDevices();
std::map<std::string, std::string> drawParamsUIForID(std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> devList, int num);
std::shared_ptr<SDRDevice> getDeviceByID(std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> devList, std::map<std::string, std::string> parameters, int num);
#endif
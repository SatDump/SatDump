#pragma once

#include "common/dsp/complex.h"
#include "common/dsp/buffer.h"
#include <vector>
#include <map>
#include <string>

#define READ_PARAMETER_IF_EXISTS_FLOAT(parameter, name) \
    if (d_parameters.count(name) > 0)                   \
        parameter = std::stof(d_parameters[name]);

enum sdr_device_type
{
    NONE,
    AIRSPY,
    RTLSDR,
    HACKRF,
    LIMESDR,
    AIRSPYHF,
    SPYSERVER,
    RTLTCP,
    FILESRC
};

class SDRDevice
{
protected:
    std::map<std::string, std::string> d_parameters;
    float d_samplerate;
    float d_frequency;

public:
    std::shared_ptr<dsp::stream<complex_t>> output_stream;
    SDRDevice(std::map<std::string, std::string> parameters, uint64_t id = 0);
    virtual std::map<std::string, std::string> getParameters() = 0;
    virtual std::string getID() = 0;
    virtual void start();
    virtual void stop();
    virtual void drawUI();
    static void init();
    virtual void setFrequency(float frequency);
    virtual void setSamplerate(float samplerate);
    virtual float getFrequency();
    virtual float getSamplerate();
    static std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> getDevices();
    static std::map<std::string, std::string> drawParamsUI();
};

void initSDRs();
std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> getAllDevices();
std::map<std::string, std::string> drawParamsUIForID(std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> devList, int num);
std::string getDeviceIDStringByID(std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> devList, int num);
sdr_device_type getDeviceIDbyIDString(std::string idString);
std::shared_ptr<SDRDevice> getDeviceByID(std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> devList, std::map<std::string, std::string> parameters, int num);

#include "sdr.h"

#ifdef BUILD_LIVE

SDRDevice::SDRDevice(std::map<std::string, std::string> parameters, uint64_t /*id*/)
    : d_parameters(parameters)
{
    output_stream = std::make_shared<dsp::stream<std::complex<float>>>();
}

void SDRDevice::start()
{
}

void SDRDevice::stop()
{
}

void SDRDevice::drawUI()
{
}

void SDRDevice::init()
{
}

void SDRDevice::setFrequency(float frequency)
{
    d_frequency = frequency;
}

void SDRDevice::setSamplerate(float samplerate)
{
    d_samplerate = samplerate;
}

int SDRDevice::getFrequency()
{
    return d_frequency;
}

int SDRDevice::getSamplerate()
{
    return d_samplerate;
}

std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> SDRDevice::getDevices()
{
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> results;
    return results;
}

std::map<std::string, std::string> SDRDevice::drawParamsUI()
{
    return std::map<std::string, std::string>();
}

#include "airspy.h"
#include "rtlsdr.h"
#include "hackrf.h"
#include "limesdr.h"
#include "spyserver.h"

void initSDRs()
{
    SDRAirspy::init();
    SDRHackRF::init();
}

std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> getAllDevices()
{
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> results;

    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> airspy_results = SDRAirspy::getDevices();
    results.insert(results.end(), airspy_results.begin(), airspy_results.end());

    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> rtlsdr_results = SDRRtlSdr::getDevices();
    results.insert(results.end(), rtlsdr_results.begin(), rtlsdr_results.end());

    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> hackrf_results = SDRHackRF::getDevices();
    results.insert(results.end(), hackrf_results.begin(), hackrf_results.end());

#ifndef _WIN32
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> limesdr_results = SDRLimeSDR::getDevices();
    results.insert(results.end(), limesdr_results.begin(), limesdr_results.end());
#endif

    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> spyserver_results = SDRSpyServer::getDevices();
    results.insert(results.end(), spyserver_results.begin(), spyserver_results.end());

    return results;
}

std::map<std::string, std::string> drawParamsUIForID(std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> devList, int num)
{
    sdr_device_type type = std::get<1>(devList[num]);

    if (type == SPYSERVER)
        return SDRSpyServer::drawParamsUI();
    else
        return std::map<std::string, std::string>();
}

std::shared_ptr<SDRDevice> getDeviceByID(std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> devList, std::map<std::string, std::string> parameters, int num)
{
    sdr_device_type type = std::get<1>(devList[num]);
    uint64_t id = std::get<2>(devList[num]);

    if (type == AIRSPY)
        return std::make_shared<SDRAirspy>(parameters, id);
    if (type == RTLSDR)
        return std::make_shared<SDRRtlSdr>(parameters, id);
    if (type == HACKRF)
        return std::make_shared<SDRHackRF>(parameters, id);
    if (type == LIMESDR)
        return std::make_shared<SDRLimeSDR>(parameters, id);
    if (type == SPYSERVER)
        return std::make_shared<SDRSpyServer>(parameters, id);
    else
        return nullptr;
}
#endif
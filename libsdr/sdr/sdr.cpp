#include "sdr.h"

SDRDevice::SDRDevice(std::map<std::string, std::string> parameters, uint64_t /*id*/)
    : d_parameters(parameters)
{
    output_stream = std::make_shared<dsp::stream<complex_t>>();
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

float SDRDevice::getFrequency()
{
    return d_frequency;
}

float SDRDevice::getSamplerate()
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
#include "airspyhf.h"
#include "sdrplay.h"
#include "spyserver.h"
#include "rtltcp.h"
#include "file.h"

void initSDRs()
{
#ifndef DISABLE_SDR_AIRSPY
    SDRAirspy::init();
#endif
#ifndef DISABLE_SDR_HACKRF
    SDRHackRF::init();
#endif
#ifndef DISABLE_SDR_SDRPLAY
    SDRPlay::init();
#endif
}

// Android support
#ifdef __ANDROID__
bool rtlsdr_device_android_ready;
int rtlsdr_device_android_fd;
std::string rtlsdr_device_android_path;

bool airspy_device_android_ready;
int airspy_device_android_fd;
std::string airspy_device_android_path;

bool airspyhf_device_android_ready;
int airspyhf_device_android_fd;
std::string airspyhf_device_android_path;
#endif

std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> getAllDevices()
{
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> results;

#ifndef DISABLE_SDR_AIRSPY
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> airspy_results = SDRAirspy::getDevices();
#ifdef __ANDROID__
    if (airspy_device_android_ready)
#endif
        results.insert(results.end(), airspy_results.begin(), airspy_results.end());
#endif

#ifndef DISABLE_SDR_RTLSDR
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> rtlsdr_results = SDRRtlSdr::getDevices();
#ifdef __ANDROID__
    if (rtlsdr_device_android_ready)
#endif
        results.insert(results.end(), rtlsdr_results.begin(), rtlsdr_results.end());
#endif

#ifndef DISABLE_SDR_HACKRF
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> hackrf_results = SDRHackRF::getDevices();
    results.insert(results.end(), hackrf_results.begin(), hackrf_results.end());
#endif

#ifndef DISABLE_SDR_LIMESDR
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> limesdr_results = SDRLimeSDR::getDevices();
    results.insert(results.end(), limesdr_results.begin(), limesdr_results.end());
#endif

#ifndef DISABLE_SDR_AIRSPYHF
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> airspyhf_results = SDRAirspyHF::getDevices();
#ifdef __ANDROID__
    if (airspyhf_device_android_ready)
#endif
        results.insert(results.end(), airspyhf_results.begin(), airspyhf_results.end());
#endif

#ifndef DISABLE_SDR_SDRPLAY
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> sdrplay_results = SDRPlay::getDevices();
    results.insert(results.end(), sdrplay_results.begin(), sdrplay_results.end());
#endif

#ifndef DISABLE_SDR_SPYSERVER
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> spyserver_results = SDRSpyServer::getDevices();
    results.insert(results.end(), spyserver_results.begin(), spyserver_results.end());
#endif

#ifndef DISABLE_SDR_RTLTCP
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> rtltcp_results = SDRRtlTcp::getDevices();
    results.insert(results.end(), rtltcp_results.begin(), rtltcp_results.end());
#endif

    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> file_results = SDRFile::getDevices();
    results.insert(results.end(), file_results.begin(), file_results.end());

    return results;
}

std::map<std::string, std::string> drawParamsUIForID(std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> devList, int num)
{
    sdr_device_type type = std::get<1>(devList[num]);

#ifndef DISABLE_SDR_SPYSERVER
    if (type == SPYSERVER)
        return SDRSpyServer::drawParamsUI();
#endif
#ifndef DISABLE_SDR_RTLTCP
    if (type == RTLTCP)
        return SDRRtlTcp::drawParamsUI();
#endif
    if (type == FILESRC)
        return SDRFile::drawParamsUI();

    return std::map<std::string, std::string>();
}

std::string deviceTypeStringByType(sdr_device_type type)
{
    if (type == AIRSPY)
        return "airspy";
    if (type == RTLSDR)
        return "rtlsdr";
    if (type == HACKRF)
        return "hackrf";
    if (type == LIMESDR)
        return "limesdr";
    if (type == AIRSPYHF)
        return "airspyhf";
    if (type == SDRPLAY)
        return "sdrplay";
    if (type == SPYSERVER)
        return "spyserver";
    if (type == RTLTCP)
        return "rtltcp";
    if (type == FILESRC)
        return "file";

    return "none";
}

std::string getDeviceIDStringByID(std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> devList, int num)
{
    sdr_device_type type = std::get<1>(devList[num]);
    return deviceTypeStringByType(type);
}

sdr_device_type getDeviceIDbyIDString(std::string idString)
{
    if (idString == "airspy")
        return AIRSPY;
    else if (idString == "rtlsdr")
        return RTLSDR;
    else if (idString == "hackrf")
        return HACKRF;
    else if (idString == "limesdr")
        return LIMESDR;
    else if (idString == "airspyhf")
        return AIRSPYHF;
    else if (idString == "sdrplay")
        return SDRPLAY;
    else if (idString == "spyserver")
        return SPYSERVER;
    else if (idString == "rtltcp")
        return RTLTCP;
    else if (idString == "file")
        return FILESRC;
    else
        return NONE;
}

std::shared_ptr<SDRDevice> getDeviceByID(std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> devList, std::map<std::string, std::string> parameters, int num)
{
    sdr_device_type type = std::get<1>(devList[num]);
    uint64_t id = std::get<2>(devList[num]);

#ifndef DISABLE_SDR_AIRSPY
    if (type == AIRSPY)
#ifdef __ANDROID__
        return std::make_shared<SDRAirspy>(parameters, airspy_device_android_fd, airspy_device_android_path);
#else
        return std::make_shared<SDRAirspy>(parameters, id);
#endif
#endif
#ifndef DISABLE_SDR_RTLSDR
    if (type == RTLSDR)
#ifdef __ANDROID__
        return std::make_shared<SDRRtlSdr>(parameters, rtlsdr_device_android_fd, rtlsdr_device_android_path);
#else
        return std::make_shared<SDRRtlSdr>(parameters, id);
#endif
#endif
#ifndef DISABLE_SDR_HACKRF
    if (type == HACKRF)
        return std::make_shared<SDRHackRF>(parameters, id);
#endif
#ifndef DISABLE_SDR_LIMESDR
    if (type == LIMESDR)
        return std::make_shared<SDRLimeSDR>(parameters, id);
#endif
#ifndef DISABLE_SDR_AIRSPYHF
    if (type == AIRSPYHF)
#ifdef __ANDROID__
        return std::make_shared<SDRAirspyHF>(parameters, airspyhf_device_android_fd, airspyhf_device_android_path);
#else
        return std::make_shared<SDRAirspyHF>(parameters, id);
#endif
#endif
#ifndef DISABLE_SDR_SDRPLAY
    if (type == SDRPLAY)
        return std::make_shared<SDRPlay>(parameters, id);
#endif
#ifndef DISABLE_SDR_SPYSERVER
    if (type == SPYSERVER)
        return std::make_shared<SDRSpyServer>(parameters, id);
#endif
#ifndef DISABLE_SDR_RTLTCP
    if (type == RTLTCP)
        return std::make_shared<SDRRtlTcp>(parameters, id);
#endif
    if (type == FILESRC)
        return std::make_shared<SDRFile>(parameters, id);

    return nullptr;
}

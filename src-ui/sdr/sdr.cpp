#include "sdr.h"

#ifdef BUILD_LIVE

SDRDevice::SDRDevice()
{
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

std::vector<std::string> SDRDevice::getDevices()
{
    std::vector<std::string> results;
    return results;
}

#include "airspy.h"
#include "hackrf.h"

void initSDRs()
{
    SDRAirspy::init();
    SDRHackRF::init();
}

std::vector<std::string> getAllDevices()
{
    std::vector<std::string> results;

    std::vector<std::string> airspy_results = SDRAirspy::getDevices();
    results.insert(results.end(), airspy_results.begin(), airspy_results.end());

    std::vector<std::string> hackrf_results = SDRHackRF::getDevices();
    results.insert(results.end(), hackrf_results.begin(), hackrf_results.end());

    return results;
}
#endif
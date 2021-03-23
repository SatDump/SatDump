#include "hackrf.h"
#include <sstream>
#include "live.h"

#include "logger.h"

#ifdef BUILD_LIVE

SDRHackRF::SDRHackRF()
{
}

void SDRHackRF::start()
{
}

void SDRHackRF::stop()
{
}

void SDRHackRF::drawUI()
{
}

void SDRHackRF::init()
{
    hackrf_init();
}

std::vector<std::string> SDRHackRF::getDevices()
{
    std::vector<std::string> results;

    hackrf_device_list_t *devs = hackrf_device_list();

    for (int i = 0; i < devs->devicecount; i++)
    {
        std::stringstream ss;
        ss << std::hex << devs->serial_numbers[i];
        results.push_back("HackRF One " + ss.str());
    }
    return results;
}

#endif
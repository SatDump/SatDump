#include "airspy.h"
#include <sstream>
#include "live.h"

#include "logger.h"

#ifdef BUILD_LIVE

SDRAirspy::SDRAirspy()
{
}

void SDRAirspy::start()
{
}

void SDRAirspy::stop()
{
}

void SDRAirspy::drawUI()
{
}

void SDRAirspy::init()
{
}

std::vector<std::string> SDRAirspy::getDevices()
{
    std::vector<std::string> results;

    uint64_t serials[100];
    int c = airspy_list_devices(serials, 100);

    for (int i = 0; i < c; i++)
    {
        std::stringstream ss;
        ss << std::hex << serials[i];
        results.push_back("AirSpy One " + ss.str());
    }
    return results;
}

#endif
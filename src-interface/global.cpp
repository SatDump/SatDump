#include "global.h"

std::shared_ptr<std::vector<std::shared_ptr<ProcessingModule>>> uiCallList;
std::shared_ptr<std::mutex> uiCallListMutex;

ctpl::thread_pool processThreadPool(8);

#ifdef BUILD_LIVE
std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> radio_devices;

bool first_init = true;
void findRadioDevices()
{
    if (first_init)
    {
        initSDRs();
        first_init = false;
    }
    radio_devices = getAllDevices();
}
#endif
#pragma once

#include "common/ctpl/ctpl_stl.h"
#include "module.h"

extern std::shared_ptr<std::vector<std::shared_ptr<ProcessingModule>>> uiCallList;
extern std::shared_ptr<std::mutex> uiCallListMutex;

extern ctpl::thread_pool processThreadPool;

#ifdef BUILD_LIVE
#include "sdr/sdr.h"
extern std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> radio_devices;
void findRadioDevices();
#endif
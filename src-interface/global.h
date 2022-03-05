#pragma once

#include "libs/ctpl/ctpl_stl.h"
#include "core/module.h"

extern std::shared_ptr<std::vector<std::shared_ptr<ProcessingModule>>> uiCallList;
extern std::shared_ptr<std::mutex> uiCallListMutex;

extern ctpl::thread_pool processThreadPool;

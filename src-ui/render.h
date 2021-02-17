#pragma once

#include <mutex>
#include "module.h"

extern std::shared_ptr<std::vector<std::shared_ptr<ProcessingModule>>> uiCallList;
extern std::shared_ptr<std::mutex> uiCallListMutex;

void render();
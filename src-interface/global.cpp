#include "global.h"

std::shared_ptr<std::vector<std::shared_ptr<ProcessingModule>>> uiCallList;
std::shared_ptr<std::mutex> uiCallListMutex;

ctpl::thread_pool processThreadPool(8);

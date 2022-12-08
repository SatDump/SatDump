#include "thread_priority.h"

#include "logger.h"
#ifdef _WIN32
#include <windows.h>
#endif

void setThreadPriority(std::thread &th, thread_priority_t priority)
{
#ifdef _WIN32
    if (SetThreadPriority(th.native_handle(), priority) == 0)
        logger->error("Could not set thread priority!");
#else
    sched_param sch_params;
    sch_params.sched_priority = priority;
    if (pthread_setschedparam(th.native_handle(), SCHED_RR, &sch_params))
        logger->error("Could not set thread priority!");
#endif
}
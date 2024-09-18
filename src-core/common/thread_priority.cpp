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
    int policy = 0;
    pthread_getschedparam(th.native_handle(), &policy, &sch_params);
    sch_params.sched_priority = priority;
    if (pthread_setschedparam(th.native_handle(), SCHED_RR, &sch_params))
        logger->error("Could not set thread priority!");
#endif
}

void setLowestThreadPriority(std::thread &th)
{
#ifdef _WIN32
    if (SetThreadPriority(th.native_handle(), -2) == 0)
        logger->error("Could not set thread priority!");
#elif defined(__APPLE__)
    sched_param sch_params;
    int policy = 0;
    pthread_getschedparam(th.native_handle(), &policy, &sch_params);
    sch_params.sched_priority = PRIORITY_LOWEST;
    if (pthread_setschedparam(th.native_handle(), SCHED_RR, &sch_params))
        logger->error("Could not set thread priority!");
#else
    sched_param sch_params;
    int policy = 0;
    pthread_getschedparam(th.native_handle(), &policy, &sch_params);
    if (pthread_setschedparam(th.native_handle(), SCHED_IDLE, &sch_params))
        logger->error("Could not set thread priority!");
#endif
}

void setLowestThreadPriority()
{
#ifdef _WIN32
    if (SetThreadPriority(GetCurrentThread(), -2) == 0)
        logger->error("Could not set thread priority!");
#elif defined(__APPLE__)
    sched_param sch_params;
    int policy = 0;
    pthread_getschedparam(pthread_self(), &policy, &sch_params);
    sch_params.sched_priority = PRIORITY_LOWEST;
    if (pthread_setschedparam(pthread_self(), SCHED_RR, &sch_params))
        logger->error("Could not set thread priority!");
#else
    sched_param sch_params;
    int policy = 0;
    pthread_getschedparam(pthread_self(), &policy, &sch_params);
    if (pthread_setschedparam(pthread_self(), SCHED_IDLE, &sch_params))
        logger->error("Could not set thread priority!");
#endif
}
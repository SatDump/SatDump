#pragma once

#include "live.h"

namespace live
{
#ifdef BUILD_LIVE
    extern bool live_processing;
    void startRealLive();
    void renderLive();
#endif
};
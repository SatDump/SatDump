#pragma once
#include "live/live.h"

namespace recorder
{
#ifdef BUILD_LIVE
    void initRecorderMenu();
    void renderRecorderMenu();
#endif
}

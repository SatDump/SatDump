#pragma once
#include "live/live.h"

namespace recorder
{
#ifdef BUILD_LIVE
    void initRecorder();
    void renderRecorder(int wwidth, int wheight);
    void exitRecorder();
#endif
};
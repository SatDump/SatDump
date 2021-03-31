#pragma once

#include "live.h"

#ifdef BUILD_LIVE
extern bool live_processing;
void startRealLive();
void renderLive();
#endif
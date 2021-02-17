#pragma once

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
#include "channels.h"

namespace jpss
{
    namespace viirs
    {
        // Simple defrag function to remove tose black lines on the edges
        void defragChannel(cimg_library::CImg<unsigned short> &image, Channel &channelSettings);
    } // namespace viirs
} // namespace jpss
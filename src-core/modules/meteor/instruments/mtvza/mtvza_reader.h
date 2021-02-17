#pragma once

#include <cstdint>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

class MTVZAReader
{
private:
    unsigned short *channels[120];

public:
    MTVZAReader();
    int lines;
    void work(uint8_t *data);
    cimg_library::CImg<unsigned short> getChannel(int channel);
};
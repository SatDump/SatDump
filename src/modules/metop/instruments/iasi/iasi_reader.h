#pragma once

#include <ccsds/ccsds.h>

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

class IASIReader
{
private:
    unsigned short *channels[8461];

public:
    IASIReader();
    int lines;
    void work(libccsds::CCSDSPacket &packet);
    cimg_library::CImg<unsigned short> getChannel(int channel);
};
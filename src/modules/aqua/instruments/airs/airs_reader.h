#pragma once

#include <ccsds/ccsds.h>

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

class AIRSReader
{
private:
    unsigned short *channels[2666];
    unsigned short *hd_channels[4];

public:
    AIRSReader();
    int lines;
    void work(libccsds::CCSDSPacket &packet);
    cimg_library::CImg<unsigned short> getChannel(int channel);
    cimg_library::CImg<unsigned short> getHDChannel(int channel);
};
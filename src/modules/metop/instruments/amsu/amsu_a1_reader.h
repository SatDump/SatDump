#pragma once

#include <ccsds/ccsds.h>

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

class AMSUA1Reader
{
private:
    unsigned short *channels[13];
    uint16_t lineBuffer[12944];

public:
    AMSUA1Reader();
    int lines;
    void work(libccsds::CCSDSPacket &packet);
    cimg_library::CImg<unsigned short> getChannel(int channel);
};
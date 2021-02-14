#pragma once

#include <ccsds/ccsds.h>

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

class IASIIMGReader
{
private:
    unsigned short *ir_channel;

public:
    IASIIMGReader();
    int lines;
    void work(libccsds::CCSDSPacket &packet);
    cimg_library::CImg<unsigned short> getIRChannel();
};
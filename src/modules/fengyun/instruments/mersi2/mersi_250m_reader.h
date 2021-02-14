#pragma once

#include <cstdint>
#include <vector>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

class MERSI250Reader
{
private:
    unsigned short *imageBuffer;
    unsigned short mersiLineBuffer[40960];
    int frames;
    uint8_t byteBufShift[3];

public:
    MERSI250Reader();
    ~MERSI250Reader();
    void pushFrame(std::vector<uint8_t>& data);
    cimg_library::CImg<unsigned short> getImage();
};
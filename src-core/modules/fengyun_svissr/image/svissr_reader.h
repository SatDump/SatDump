#pragma once

#include <cstdint>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

namespace fengyun_svissr
{
    class SVISSRReader
    {
    public:
        unsigned short *imageBufferIR1, *imageBufferIR2, *imageBufferIR3, *imageBufferIR4, *imageBufferVIS;

    private:
        unsigned short *imageLineBuffer;
        uint8_t byteBufShift[5];
        bool *goodLines;

    public:
        SVISSRReader();
        ~SVISSRReader();
        void pushFrame(uint8_t *data);
        void reset();
        cimg_library::CImg<unsigned short> getImage();
        cimg_library::CImg<unsigned short> getImageIR1();
        cimg_library::CImg<unsigned short> getImageIR2();
        cimg_library::CImg<unsigned short> getImageIR3();
        cimg_library::CImg<unsigned short> getImageIR4();
        cimg_library::CImg<unsigned short> getImageVIS();
    };
}
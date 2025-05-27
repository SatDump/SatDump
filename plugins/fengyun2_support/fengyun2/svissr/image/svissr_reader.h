#pragma once

#include <cstdint>
#include "image/image.h"

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
        image::Image getImage();
        image::Image getImageIR1();
        image::Image getImageIR2();
        image::Image getImageIR3();
        image::Image getImageIR4();
        image::Image getImageVIS();
    };
}
#pragma once

#include <cstdint>
#include "common/image/image.h"

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
        image::Image<uint16_t> getImage();
        image::Image<uint16_t> getImageIR1();
        image::Image<uint16_t> getImageIR2();
        image::Image<uint16_t> getImageIR3();
        image::Image<uint16_t> getImageIR4();
        image::Image<uint16_t> getImageVIS();
    };
}
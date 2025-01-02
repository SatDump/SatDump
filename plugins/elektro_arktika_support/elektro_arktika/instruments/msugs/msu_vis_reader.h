#pragma once

#include <cstdint>
#include "common/image/image.h"

namespace elektro_arktika
{
    namespace msugs
    {
        class MSUVISReader
        {
        private:
            unsigned short *imageBuffer1, *imageBuffer2;
            unsigned short msuLineBuffer[12044];

        public:
            int frames;

        public:
            MSUVISReader();
            ~MSUVISReader();
            void pushFrame(uint8_t *data, int offset);
            image::Image getImage1();
            image::Image getImage2();
        };
    } // namespace msugs
} // namespace elektro_arktika
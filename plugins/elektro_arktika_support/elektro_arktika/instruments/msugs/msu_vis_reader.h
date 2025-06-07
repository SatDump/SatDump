#pragma once

#include <cstdint>
#include "image/image.h"

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
            std::vector<double> timestamps;

        public:
            MSUVISReader();
            ~MSUVISReader();
            void pushFrame(uint8_t *data);
            image::Image getImage1();
            image::Image getImage2();
        };
    } // namespace msugs
} // namespace elektro_arktika
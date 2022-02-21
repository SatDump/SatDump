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
            unsigned short *imageBuffer;
            unsigned short msuLineBuffer[12044];
            int frames;

        public:
            MSUVISReader();
            ~MSUVISReader();
            void pushFrame(uint8_t *data);
            image::Image<uint16_t> getImage();
        };
    } // namespace msugs
} // namespace elektro_arktika
#pragma once

#include <cstdint>
#include "image/image.h"

namespace elektro_arktika
{
    namespace msugs
    {
        class MSUIRReader
        {
        private:
            unsigned short *imageBuffer[7];
            unsigned short msuLineBuffer[12044];

        public:
            int frames;

        public:
            MSUIRReader();
            ~MSUIRReader();
            void pushFrame(uint8_t *data);
            image::Image getImage(int channel);
        };
    } // namespace msugs
} // namespace elektro_arktika
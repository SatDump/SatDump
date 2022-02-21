#pragma once

#include <cstdint>
#include "common/image/image.h"

namespace elektro_arktika
{
    namespace msugs
    {
        class MSUIRReader
        {
        private:
            unsigned short *imageBuffer[7];
            unsigned short msuLineBuffer[12044];
            int frames;

        public:
            MSUIRReader();
            ~MSUIRReader();
            void pushFrame(uint8_t *data);
            image::Image<uint16_t> getImage(int channel);
        };
    } // namespace msugs
} // namespace elektro_arktika
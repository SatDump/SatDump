#pragma once

#include <cstdint>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

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
            cimg_library::CImg<unsigned short> getImage(int channel);
            cimg_library::CImg<unsigned short> getImage2(int channel);
        };
    } // namespace msugs
} // namespace elektro_arktika
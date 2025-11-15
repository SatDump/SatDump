#pragma once

#include "image/image.h"
#include <cstdint>

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

            // Related to counter correction
            int global_counter;
            bool counter_locked = false;

        public:
            MSUVISReader();
            ~MSUVISReader();
            void pushFrame(uint8_t *data, bool apply_correction);
            image::Image getImage1();
            image::Image getImage2();
        };
    } // namespace msugs
} // namespace elektro_arktika
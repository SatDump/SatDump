#pragma once

#include <cstdint>
#include "common/image/image.h"

namespace goes
{
    namespace gvar
    {
        class VisibleReader
        {
        public:
            unsigned short *imageBuffer;
            unsigned short *imageLineBuffer;

        private:
            uint8_t byteBufShift[5];
            bool *goodLines;

        public:
            int frames;
            VisibleReader();
            ~VisibleReader();
            void startNewFullDisk();
            void pushFrame(uint8_t *data, int block, int counter);
            image::Image<uint16_t> getImage();
        };
    }
}
#pragma once

#include <cstdint>
#include "common/image/image.h"

namespace goes
{
    namespace gvar
    {
        class InfraredReader1
        {
        public:
            unsigned short *imageBuffer1;
            unsigned short *imageBuffer2;
            unsigned short *imageLineBuffer;

        private:
            bool *goodLines;

        public:
            int frames;
            InfraredReader1();
            ~InfraredReader1();
            void startNewFullDisk();
            void pushFrame(uint8_t *data, int counter, int word_cnt);
            image::Image<uint16_t> getImage1();
            image::Image<uint16_t> getImage2();
        };
    }
}
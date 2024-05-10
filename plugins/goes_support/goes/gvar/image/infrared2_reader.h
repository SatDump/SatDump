#pragma once

#include <cstdint>
#include "common/image/image.h"

namespace goes
{
    namespace gvar
    {
        class InfraredReader2
        {
        public:
            unsigned short *imageBuffer1;
            unsigned short *imageBuffer2;
            unsigned short *imageLineBuffer;

        private:
            bool *goodLines;

        public:
            int frames;
            InfraredReader2();
            ~InfraredReader2();
            void startNewFullDisk();
            void pushFrame(uint8_t *data, int counter, int word_cnt);
            image::Image getImage1();
            image::Image getImage2();
        };
    }
}
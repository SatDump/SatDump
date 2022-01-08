#pragma once

#include <cstdint>
#include "simpledeframer.h"
#include "common/image/image.h"

namespace noaa
{
    namespace amsu
    {
        class AMSUReader
        {
        private:
            unsigned short *channels[15];
            SimpleDeframer<uint32_t, 24, 312 * 8, 0xFFFFFF> amsuA2Deframer;
            SimpleDeframer<uint32_t, 24, 1240 * 8, 0xFFFFFF> amsuA1Deframer;
            std::vector<std::vector<uint8_t>> amsuA2Data;
            std::vector<std::vector<uint8_t>> amsuA1Data;

        public:
            AMSUReader();
            ~AMSUReader();
            int linesA1 = 0;
            int linesA2 = 0;
            void work(uint8_t *buffer);
            image::Image<uint16_t> getChannel(int channel);
        };
    } // namespace amsu
} // namespace noaa
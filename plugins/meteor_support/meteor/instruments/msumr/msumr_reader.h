#pragma once

#include <cstdint>
#include <vector>
#include "image/image.h"

namespace meteor
{
    namespace msumr
    {
        class MSUMRReader
        {
        private:
            std::vector<uint16_t> channels[6];

        public:
            std::vector<uint16_t> calibration_info[6][2];

        public:
            MSUMRReader();
            ~MSUMRReader();
            int lines;
            void work(uint8_t *buffer);
            image::Image getChannel(int channel);
        };
    } // namespace msumr
} // namespace meteor
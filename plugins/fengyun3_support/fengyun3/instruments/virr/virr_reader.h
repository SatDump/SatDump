#pragma once

#include <cstdint>
#include "common/image/image.h"
#include <vector>
#include "common/resizeable_buffer.h"

namespace fengyun3
{
    namespace virr
    {
        class VIRRReader
        {
        private:
            ResizeableBuffer<unsigned short> channels[10];
            uint16_t virrBuffer[204800];

        public:
            VIRRReader();
            ~VIRRReader();
            int lines;
            int day_offset = 0;
            std::vector<double> timestamps;
            void work(std::vector<uint8_t> &packet);
            image::Image<uint16_t> getChannel(int channel);
        };
    } // namespace virr
} // namespace fengyun
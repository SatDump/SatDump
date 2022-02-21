#pragma once

#include <cstdint>
#include "common/image/image.h"
#include <vector>
#include "common/resizeable_buffer.h"

namespace fengyun3
{
    namespace mwri
    {
        class MWRIReader
        {
        private:
            ResizeableBuffer<unsigned short> channels[10];

        public:
            MWRIReader();
            ~MWRIReader();
            int lines;
            void work(std::vector<uint8_t> &packet);
            image::Image<uint16_t> getChannel(int channel);
        };
    } // namespace virr
} // namespace fengyun
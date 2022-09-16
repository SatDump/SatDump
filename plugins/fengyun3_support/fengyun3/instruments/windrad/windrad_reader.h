#pragma once

#include <cstdint>
#include "common/image/image.h"
#include <vector>
#include "common/resizeable_buffer.h"
#include <string>

namespace fengyun3
{
    namespace windrad
    {
        class WindRADReader
        {
        private:
            const int width;
            const std::string band;
            const std::string directory;
            ResizeableBuffer<unsigned short> channels[2];
            int lastMarker = 0;

        public:
            WindRADReader(int width, std::string bnd, std::string dir);
            ~WindRADReader();
            int lines;
            int imgCount = 0;
            void work(std::vector<uint8_t> &packet);
            image::Image<uint16_t> getChannel(int channel);
        };
    } // namespace virr
} // namespace fengyun
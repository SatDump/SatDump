#pragma once

#include <cstdint>
#include "image/image.h"
#include <vector>

namespace fengyun3
{
    namespace mwri
    {
        class MWRIReader
        {
        private:
            std::vector<unsigned short> channels[10];

        public:
            MWRIReader();
            ~MWRIReader();
            int lines;
            void work(std::vector<uint8_t> &packet);
            image::Image getChannel(int channel);

            std::vector<double> timestamps;
        };
    } // namespace virr
} // namespace fengyun
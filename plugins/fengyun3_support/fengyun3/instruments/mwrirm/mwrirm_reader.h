#pragma once

#include <cstdint>
#include "common/image2/image.h"
#include <vector>
#include "common/resizeable_buffer.h"

namespace fengyun3
{
    namespace mwrirm
    {
        class MWRIRMReader
        {
        private:
            std::vector<unsigned short> channels[26];

        public:
            MWRIRMReader();
            ~MWRIRMReader();
            int lines;
            void work(std::vector<uint8_t> &packet);
            image2::Image getChannel(int channel);

            std::vector<double> timestamps;
        };
    } // namespace virr
} // namespace fengyun
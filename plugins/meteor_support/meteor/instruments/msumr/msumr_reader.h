#pragma once

#include <cstdint>
#include <vector>
#include "common/image2/image.h"

namespace meteor
{
    namespace msumr
    {
        class MSUMRReader
        {
        private:
            std::vector<uint16_t> channels[6];

        public:
            MSUMRReader();
            ~MSUMRReader();
            int lines;
            void work(uint8_t *buffer);
            image2::Image getChannel(int channel);
        };
    } // namespace msumr
} // namespace meteor
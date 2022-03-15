#pragma once

#include <cstdint>
#include "common/image/image.h"

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
            image::Image<uint16_t> getChannel(int channel);
        };
    } // namespace msumr
} // namespace meteor
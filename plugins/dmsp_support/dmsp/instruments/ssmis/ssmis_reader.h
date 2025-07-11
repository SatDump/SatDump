#pragma once

#include "image/image.h"
#include <cstdint>
#include <vector>

namespace dmsp
{
    namespace ssmis
    {
        class SSMISReader
        {
        private:
            std::vector<uint16_t> ssmis_data[24];

        public:
            SSMISReader();
            ~SSMISReader();

            int lines;

            std::vector<double> timestamps;

            void work(uint8_t *ssmis_frame, double last_timestamp);

            image::Image getChannel(int ch);
        };
    } // namespace ssmis
} // namespace dmsp
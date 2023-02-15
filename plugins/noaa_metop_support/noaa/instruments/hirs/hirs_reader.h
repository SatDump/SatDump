#pragma once

#include <cstdint>
#include "common/image/image.h"
#include "../../tip_time_parser.h"
#include "../../contains.h"

namespace noaa
{
    namespace hirs
    {
        class HIRSReader
        {
        private:
            std::vector<uint16_t> channels[20];
            //std::vector<uint16_t> imageBuffer[20];
            const int HIRSPositions[36] = {16, 17, 22, 23, 26, 27, 30, 31, 34, 35, 38, 39, 42, 43, 54, 55, 58, 59, 62, 63, 66, 67, 70, 71, 74, 75, 78, 79, 82, 83, 84, 85, 88, 89, 92, 93};
            const int HIRSChannels[20] = {0, 16, 1, 2, 12, 3, 17, 10, 18, 6, 7, 19, 9, 13, 5, 4, 14, 11, 15, 8};
            unsigned int last = 0;

        public:
            HIRSReader();
            ~HIRSReader();
            int line = 0;
            void work(uint8_t *buffer);
            image::Image<uint16_t> getChannel(int channel);

            double last_timestamp = -1;
            TIPTimeParser ttp;
            std::vector<double> timestamps;
            int sync = 0;
        };
    } // namespace hirs
} // namespace noaa
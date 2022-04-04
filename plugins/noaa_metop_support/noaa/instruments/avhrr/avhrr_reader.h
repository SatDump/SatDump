#pragma once

#include <cstdint>
#include "common/image/image.h"

namespace noaa
{
    namespace avhrr
    {
        class AVHRRReader
        {
        private:
            const bool gac_mode;
            const int width;

            unsigned short *channels[5];

            time_t dayYearValue = 0;

        public:
            AVHRRReader(bool gac);
            ~AVHRRReader();
            int lines;
            std::vector<int> spacecraft_ids;
            std::vector<double> timestamps;
            void work(uint16_t *buffer);
            image::Image<uint16_t> getChannel(int channel);
        };
    } // namespace avhrr
} // namespace noaa
#pragma once

#include "common/ccsds/ccsds.h"
#include "image/image.h"
#include <cmath>

namespace metop
{
    namespace ascat
    {
        inline double parse_uint_to_float(uint16_t sample)
        {
            // Details of this format from pyascatreader.cpp
            bool s = (sample >> 15) & 0x1;
            unsigned int e = (sample >> 7) & 0xff;
            unsigned int f = (sample) & 0x7f;

            double value = 0;

            if (e == 255)
            {
                value = 0.0;
            }
            else if (e == 0)
            {
                if (f == 0)
                    value = 0.0;
                else
                    value = (s ? -1.0 : 1.0) * pow(2.0, -126.0) * (double)f / 128.0;
            }
            else
            {
                value = (s ? -1.0 : 1.0) * pow(2.0, double(e - 127)) * ((double)f / 128.0 + 1.0);
            }
            return value;
        }

        class ASCATReader
        {
        public:
            std::vector<std::vector<float>> channels[6];
            std::vector<uint16_t> channels_img[6];
            int lines[6];
            std::vector<double> timestamps[6];

            std::vector<std::vector<float>> noise_channels[6];
            int noise_lines[6];
            std::vector<double> noise_timestamps[6];

        public:
            ASCATReader();
            ~ASCATReader();
            void work(ccsds::CCSDSPacket &packet);
            image::Image getChannelImg(int channel);
            std::vector<std::vector<float>> getChannel(int channel);
        };
    } // namespace ascat
} // namespace metop
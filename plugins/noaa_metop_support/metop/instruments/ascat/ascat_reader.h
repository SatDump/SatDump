#pragma once

#include "common/ccsds/ccsds.h"
#include "image/image.h"

namespace metop
{
    namespace ascat
    {
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
    } // namespace avhrr
} // namespace metop
#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include <map>
#include "common/image/image.h"

namespace jason3
{
    namespace amr2
    {
        class AMR2Reader
        {
        private:
            std::vector<uint16_t> channels[3];

        public:
            AMR2Reader();
            ~AMR2Reader();

            int lines;
            std::vector<double> timestamps;

            std::vector<double> timestamps_data;
            std::vector<double> channels_data[3];

            void work(ccsds::CCSDSPacket &packet);
            image::Image getChannel(int channel);
        };
    } // namespace modis
} // namespace eos
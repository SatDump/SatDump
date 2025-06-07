#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include "image/image.h"
extern "C"
{
#include <libs/aec/szlib.h>
}

namespace jpss
{
    namespace omps
    {
        class OMPSLimbReader
        {
        private:
            std::vector<uint16_t> channels[135];
            uint8_t *finalFrameVector;
            std::vector<uint8_t> currentOMPSFrame;
            SZ_com_t rice_parameters;

        public:
            OMPSLimbReader();
            ~OMPSLimbReader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            image::Image getChannel(int channel);
        };
    } // namespace atms
} // namespace jpss
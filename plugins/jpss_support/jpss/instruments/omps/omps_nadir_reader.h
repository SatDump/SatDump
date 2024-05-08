#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include "common/image2/image.h"
extern "C"
{
#include <libs/aec/szlib.h>
}

namespace jpss
{
    namespace omps
    {
        class OMPSNadirReader
        {
        private:
            std::vector<uint16_t> channels[339];
            uint8_t *finalFrameVector;
            std::vector<uint8_t> currentOMPSFrame;
            SZ_com_t rice_parameters;

        public:
            OMPSNadirReader();
            ~OMPSNadirReader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            image2::Image getChannel(int channel);
        };
    } // namespace atms
} // namespace jpss
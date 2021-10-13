#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
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
            unsigned short *channels[339];
            uint8_t *finalFrameVector;
            std::vector<uint8_t> currentOMPSFrame;
            SZ_com_t rice_parameters;

        public:
            OMPSNadirReader();
            ~OMPSNadirReader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getChannel(int channel);
        };
    } // namespace atms
} // namespace jpss
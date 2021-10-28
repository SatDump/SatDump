#pragma once

#include "common/ccsds/ccsds.h"
#include <tuple>

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

#include "lrpt/segment.h"
#include <array>

namespace meteor
{
    namespace msumr
    {
        namespace lrpt
        {
            class MSUMRReader
            {
            private:
                unsigned short *channels[6];
                Segment *segments[6];
                uint32_t rollover[6], lastSeq[6], offset[6], firstSeg[6], lastSeg[6], segCount[6];
                int lines[6];

                time_t dayValue;

            public:
                MSUMRReader();
                ~MSUMRReader();
                void work(ccsds::CCSDSPacket &packet);
                cimg_library::CImg<unsigned short> getChannel(int channel, int32_t first = -1, int32_t last = -1, int32_t offset = 1);
                std::array<int32_t, 3> correlateChannels(int channel1, int channel2);
                std::array<int32_t, 3> correlateChannels(int channel1, int channel2, int channel3);

                std::vector<double> timestamps;
            };
        } // namespace lrpt
    }     // namespace msumr
} // namespace meteor
#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include "packets.h"
#include <vector>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
#include <memory>

namespace jpss
{
    namespace viirs
    {
        // Reader to parse a VIIRS channel
        class VIIRSReader
        {
        private:
            bool foundData;
            uint16_t endSequenceCount;
            int currentSegment;
            std::shared_ptr<unsigned short> imageBuffer;
            int lines;

        public:
            std::vector<Segment> segments;
            Channel channelSettings;
            VIIRSReader(Channel &ch);
            ~VIIRSReader();
            void feed(ccsds::CCSDSPacket &packet);
            void differentialDecode(VIIRSReader &channelSource, int deci);
            std::vector<double> timestamps;
            cimg_library::CImg<unsigned short> getImage();
        };
    } // namespace viirs
} // namespace jpss
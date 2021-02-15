#pragma once

#include <ccsds/ccsds.h>
#include <cmath>
#include "packets.h"
#include <vector>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

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
            unsigned short *imageBuffer;
            int lines;

        public:
            std::vector<Segment> segments;
            Channel channelSettings;
            VIIRSReader(Channel &ch);
            void feed(libccsds::CCSDSPacket &packet);
            void process();
            void differentialDecode(VIIRSReader &channelSource, int deci);
            cimg_library::CImg<unsigned short> getImage();
        };
    } // namespace viirs
} // namespace jpss
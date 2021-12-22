#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include "packets.h"
#include <vector>
#include "common/image/image.h"
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
            image::Image<uint16_t> getImage();
        };
    } // namespace viirs
} // namespace jpss
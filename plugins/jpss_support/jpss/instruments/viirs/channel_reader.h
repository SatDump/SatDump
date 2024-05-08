#pragma once

#include "common/ccsds/ccsds.h"
#include <vector>
#include <array>
#include "common/image/image.h"
#include "channels.h"

extern "C"
{
#include "libs/aec/libaec.h"
}

namespace jpss
{
    namespace viirs
    {
        struct VIIRS_Segment
        {
            double timestamp;
            std::array<std::array<std::vector<uint16_t>, 6>, 32> detector_data;

            VIIRS_Segment(Channel ch)
            {
                for (int i = 0; i < ch.zoneHeight; i++)
                    for (int y = 0; y < 6; y++)
                        detector_data[i][y].resize(ch.zoneWidth[y] * ch.oversampleZone[y], 0);
            }
        };

        // Reader to parse a VIIRS channel
        class VIIRSReader
        {
        private:
            bool in_segment;
            uint16_t endSequenceCount;
            int currentSegment;

            VIIRS_Segment current_segment;

            aec_stream aec_cfg;

            void bit_slicer_detector(int &len, int fillsize)
            {
                int bits = 0, bytes = 0;

                while (fillsize % 8 != 0)
                {
                    bits++;
                    fillsize--;
                }

                bytes = len - (fillsize / 8);

                if (bytes > len || bytes < 0)
                    return;

                len = bytes + 1;
            }

            VIIRS_Segment &get_current_seg()
            {
                return segments[segments.size() - 1];
            }

        public:
            std::vector<VIIRS_Segment> segments;
            Channel channelSettings;

        public:
            VIIRSReader(Channel &ch);
            ~VIIRSReader();
            void feed(ccsds::CCSDSPacket &packet);
            void differentialDecode(VIIRSReader &channelSource, int decimation);

            std::vector<double> timestamps;
            image::Image getImage();
        };
    } // namespace viirs
} // namespace jpss
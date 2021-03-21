#include "channel_correlator.h"

namespace jpss
{
    namespace viirs
    {
        std::pair<VIIRSReader, VIIRSReader> correlateChannels(VIIRSReader segments1, VIIRSReader segments2)
        {
            // Output vectors
            VIIRSReader segments1_out(segments1.channelSettings), segments2_out(segments2.channelSettings);

            // Check all segments, only pushing commons ones
            for (int i = 0; i < (int)segments1.segments.size(); i++)
            {
                Segment &seg1 = segments1.segments[i];

                for (int y = 0; y < (int)segments2.segments.size(); y++)
                {
                    Segment &seg2 = segments2.segments[y];

                    if (seg1.header.time_start_of_scan == seg2.header.time_start_of_scan)
                    {
                        segments1_out.segments.push_back(seg1);
                        segments2_out.segments.push_back(seg2);
                        break;
                    }
                }
            }

            return {segments1_out, segments2_out};
        }

        std::tuple<VIIRSReader, VIIRSReader, VIIRSReader> correlateThreeChannels(VIIRSReader segments1, VIIRSReader segments2, VIIRSReader segments3)
        {
            // Output vectors
            VIIRSReader segments1_out(segments1.channelSettings), segments2_out(segments2.channelSettings), segments3_out(segments3.channelSettings);

            // Check all segments, only pushing commons ones
            for (int i = 0; i < (int)segments1.segments.size(); i++)
            {
                Segment &seg1 = segments1.segments[i];

                for (int y = 0; y < (int)segments2.segments.size(); y++)
                {
                    Segment &seg2 = segments2.segments[y];

                    if (seg1.header.time_start_of_scan == seg2.header.time_start_of_scan)
                    {
                        for (int y = 0; y < (int)segments3.segments.size(); y++)
                        {
                            Segment &seg3 = segments3.segments[y];

                            if (seg2.header.time_start_of_scan == seg3.header.time_start_of_scan)
                            {
                                segments1_out.segments.push_back(seg1);
                                segments2_out.segments.push_back(seg2);
                                segments3_out.segments.push_back(seg3);
                                break;
                            }
                        }
                        break;
                    }
                }
            }

            return {segments1_out, segments2_out, segments3_out};
        }
    } // namespace viirs
} // namespace jpss
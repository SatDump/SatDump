#pragma once

/**
 * @file lrpt_msumr_reader.h
 */

#include <set>
#include <array>
#include <map>
#include "common/ccsds/ccsds.h"
#include "image/image.h"
#include "lrpt/segment.h"

namespace meteor
{
    namespace msumr
    {
        namespace lrpt
        {
            /**
             * @brief LRPT MSU-MR decoder
             *
             * Takes LRPT CCSDS packets and arranges them into images
             *
             * @param segments An array of maps holding all the 112*8 segments of the image
             * @param firstSeg An array holding the first segment received for each channel
             * @param rollover An array that tracks when the sequence counter for each channel
             *                 rolls over
             * @param offset An array tracking what the sequence counter was when this
             *               channel first appeared after sequence count 0
             * @param lastSeq An array holding the last segment received for each channel
             * @param dayValue M2 LRPT Only - the day of the transmission, Moscow time
             * @param meteorm2x_mode Switches the class between decoding M2 and M2-x
             * @param lines the number of lines in the channel. Only valid after calling getChannel
             * @param timestamps the timestamps for the given channel. Only valid after calling getChannel
             */
            class MSUMRReader
            {
            private:
                std::map<uint32_t, Segment> segments[6];
                uint32_t firstSeg[6], rollover[6], lastSeq[6], offset[6], lastSeg[6];

                time_t dayValue;

                const bool meteorm2x_mode;

            public:
                /**
                 * @brief Constructor for MSUMRReader
                 *
                 * @param meteorm2x_mode Select between decoding M2 and M2-x
                 */
                MSUMRReader(bool meteorm2x_mode);
                ~MSUMRReader();

                /**
                 * @brief Reads ccsds packets and stores the processed segment into segments
                 *
                 * @param packet The CCSDS packet to process
                 */
                void work(ccsds::CCSDSPacket &packet);

                /**
                 * @brief Converts segments in the requested channel into an image
                 *
                 * The returned image is aligned with other channels that were also received
                 *
                 * @param channel the zero-indexed channel to return
                 * @param max_correct the number of missing lines to fill in. Set to 0 for no fill
                 *
                 * @return the channel image
                 */
                image::Image getChannel(int channel, size_t max_correct = 0);

                int lines[6];
                std::vector<double> timestamps;
            };
        } // namespace lrpt
    }     // namespace msumr
} // namespace meteor
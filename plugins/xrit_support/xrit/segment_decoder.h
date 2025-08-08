#pragma once

/**
 * @file segment_decoder.h
 * @brief Base segmented decoder class
 */

#include "image/image.h"
#include "xrit/identify.h"
#include "xrit/xrit_file.h"
#include <cstring>

namespace satdump
{
    namespace xrit
    {
        /**
         * @brief Abstract Segmented decoder implementation
         * to be overriden for mission-specific decoders
         * @param image WIP image
         * @param info xRIT File Info, to be set by the user
         * on creating an instance of this class, and used when
         * saving the final image.
         */
        class SegmentedImageDecoder
        {
        public:
            /**
             * @brief Push a segment into the work-in-progress image.
             * These should already be filtered using file_info.seg_groupid
             * @param file segment to push
             */
            virtual void pushSegment(XRITFile &file) = 0;

            /**
             * @brief Allows knowing if an image is complete, to save it
             * instantly.
             * @return true if complete
             */
            virtual bool isComplete() = 0;

            /**
             * @brief Resets the decoder, to lower RAM usage
             */
            virtual void reset() = 0;

            /**
             * @brief Allows knowing if an image has data at
             * all (and hence, worth saving)
             * @return true if at least 1 segment is present
             */
            virtual bool hasData() = 0;

        public:
            image::Image image;
            XRITFileInfo info;
        };
    } // namespace xrit
} // namespace satdump
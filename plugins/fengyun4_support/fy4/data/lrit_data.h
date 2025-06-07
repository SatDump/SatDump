#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include "image/image.h"
#include <vector>
#include <string>
#include <map>
#include <memory>

namespace fy4
{
    namespace lrit
    {
        class SegmentedLRITImageDecoder
        {
        private:
            int seg_count = 0;
            std::shared_ptr<bool> segments_done;
            int seg_height = 0, seg_width = 0;

        public:
            SegmentedLRITImageDecoder(int max_seg, int segment_width, int segment_height, std::string id);
            SegmentedLRITImageDecoder();
            ~SegmentedLRITImageDecoder();
            void pushSegment(image::Image &data, int segc, int height);
            bool isComplete();
            image::Image image;
            std::string image_id = "";
        };

        enum lrit_image_status
        {
            RECEIVING,
            SAVING,
            IDLE
        };
    } // namespace atms
} // namespace jpss
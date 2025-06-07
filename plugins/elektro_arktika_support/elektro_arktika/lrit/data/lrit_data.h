#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include "image/image.h"
#include <vector>
#include <string>
#include <map>
#include <memory>
#include "common/lrit/lrit_file.h"

namespace elektro
{
    namespace lrit
    {
        struct GOMSxRITProductMeta
        {
            std::string filename;

            int bit_depth = -1;
            int channel = -1;
            std::string satellite_name;
            std::string satellite_short_name;
            time_t scan_time = 0;
            std::shared_ptr<::lrit::ImageNavigationRecord> image_navigation_record;
        };

        enum lrit_image_status
        {
            RECEIVING,
            SAVING,
            IDLE
        };

        class SegmentedLRITImageDecoder
        {
        private:
            int seg_count = 0;
            std::shared_ptr<bool> segments_done;
            int seg_height = 0, seg_width = 0;

        public:
            SegmentedLRITImageDecoder(int bit_depth, int max_seg, int segment_width, int segment_height, std::string id);
            SegmentedLRITImageDecoder();
            ~SegmentedLRITImageDecoder();
            void pushSegment(image::Image &data, int segc);
            bool isComplete();
            image::Image image;
            std::string image_id = "";
            GOMSxRITProductMeta meta;
        };
    } // namespace atms
} // namespace jpss
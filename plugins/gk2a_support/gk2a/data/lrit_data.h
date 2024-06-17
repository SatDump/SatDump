#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include "common/image/image.h"
#include <vector>
#include <string>
#include <map>
#include <memory>
#include "common/lrit/lrit_file.h"

namespace gk2a
{
    namespace lrit
    {
        struct GK2AxRITProductMeta
        {
            std::string filename;

            std::string channel;
            std::string satellite_name;
            std::string satellite_short_name;
            time_t scan_time = 0;
            std::shared_ptr<::lrit::ImageNavigationRecord> image_navigation_record;
            std::shared_ptr<::lrit::ImageDataFunctionRecord> image_data_function_record;
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

            GK2AxRITProductMeta meta;
        };

        enum lrit_image_status
        {
            RECEIVING,
            SAVING,
            IDLE
        };
    } // namespace atms
} // namespace jpss
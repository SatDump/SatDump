#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include "common/image/image.h"
#include <vector>
#include <string>
#include <map>
#include <memory>
#include "common/lrit/lrit_file.h"
#include <string>

namespace goes
{
    namespace hrit
    {
        struct GOESxRITProductMeta
        {
            std::string filename;
            bool is_goesn = false;
            std::string region = "Others";
            int channel = -1;
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
            int seg_size = 0;

        public:
            SegmentedLRITImageDecoder(int max_seg, int max_width, int max_height, uint16_t id);
            SegmentedLRITImageDecoder();
            ~SegmentedLRITImageDecoder();
            void pushSegment(uint8_t *data, size_t this_size, int segc);
            bool isComplete();
            std::shared_ptr<image::Image> image;
            int image_id = -1;
            GOESxRITProductMeta meta;
        };

        enum lrit_image_status
        {
            RECEIVING,
            SAVING,
            IDLE
        };
    } // namespace hrit
} // namespace goes
#pragma once

#include <cmath>
#include "common/image/image.h"
#include <vector>
#include <string>
#include <map>
#include <memory>
#include "common/lrit/lrit_file.h"

namespace himawari
{
    namespace himawaricast
    {
        struct HIMxRITProductMeta
        {
            std::string filename;
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
            int seg_height = 0, seg_width = 0;

        public:
            SegmentedLRITImageDecoder()
            {
                seg_count = 0;
                seg_height = 0;
                seg_width = 0;
                image_id = -1;
            }

            SegmentedLRITImageDecoder(int max_seg, int segment_width, int segment_height, long id) : seg_count(max_seg), image_id(id)
            {
                segments_done = std::shared_ptr<bool>(new bool[seg_count], [](bool *p)
                                                      { delete[] p; });
                std::fill(segments_done.get(), &segments_done.get()[seg_count], false);

                image = image::Image<uint16_t>(segment_width, segment_height * max_seg, 1);
                seg_height = segment_height;
                seg_width = segment_width;

                image.fill(0);
            }

            ~SegmentedLRITImageDecoder()
            {
            }

            void pushSegment(uint16_t *data, int segc)
            {
                if (segc >= seg_count)
                    return;
                std::memcpy(&image[(seg_height * seg_width) * segc], data, seg_height * seg_width * 2);
                segments_done.get()[segc] = true;
            }

            bool isComplete()
            {
                bool complete = true;
                for (int i = 0; i < seg_count; i++)
                    complete = complete && segments_done.get()[i];
                return complete;
            }

            image::Image<uint16_t> image;
            long image_id = -1;

            HIMxRITProductMeta meta;
        };
    }
}
#pragma once

#include "../segment_decoder.h"
#include "common/lrit/lrit_file.h"
#include "image/image.h"
#include "utils/string.h"
#include "xrit/identify.h"
#include "xrit/processor/get_img.h"

namespace satdump
{
    namespace xrit
    {
        class GK2ASegmentedImageDecoder : public SegmentedImageDecoder
        {
        private:
            int seg_count = 0;
            std::vector<bool> segments_done;
            int seg_height = 0, seg_width = 0;

        public:
            void init(int bit_depth, int max_seg, int segment_width, int segment_height)
            {
                seg_count = max_seg;
                segments_done.resize(seg_count, false);

                image = image::Image(bit_depth, segment_width, segment_height * max_seg, 1);
                seg_height = segment_height;
                seg_width = segment_width;

                image.fill(0);
            }

            GK2ASegmentedImageDecoder(lrit::LRITFile &file)
            {
                lrit::ImageStructureRecord image_structure_record = file.getHeader<::lrit::ImageStructureRecord>();

                init(image_structure_record.bit_per_pixel > 8 ? 16 : 8, //
                     10,                                                //
                     image_structure_record.columns_count,              //
                     image_structure_record.lines_count);
            }

            void pushSegment(image::Image &data, int segc)
            {
                if (segc >= seg_count || segc < 0)
                    return;
                if (data.size() != seg_height * seg_width)
                {
                    logger->error("Image of the wrong size!");
                    return;
                }
                image::imemcpy(image, (seg_height * seg_width) * segc, data, 0, seg_height * seg_width);
                segments_done[segc] = true;
            }

            void pushSegment(lrit::LRITFile &file)
            {
                auto img = getImageFromXRITFile(XRIT_GK2A_AMI, file);

                std::vector<std::string> header_parts = splitString(file.filename, '_');

                int seg_number = 0;
                if (header_parts.size() >= 7)
                    seg_number = std::stoi(header_parts[6].substr(0, header_parts.size() - 4)) - 1;
                else
                {
                    logger->critical("Could not parse segment number from filename!");
                    return;
                }

                pushSegment(img, seg_number);
            }

            bool isComplete()
            {
                bool complete = true;
                for (int i = 0; i < seg_count; i++)
                    complete = complete && segments_done[i];
                return complete;
            }

            void reset()
            {
                for (int i = 0; i < seg_count; i++)
                    segments_done[i] = false;
                image.clear();
            }

            bool hasData()
            {
                for (int i = 0; i < seg_count; i++)
                    if (segments_done[i])
                        return true;
                return false;
            }
        };
    } // namespace xrit
} // namespace satdump
#pragma once

#include "../segment_decoder.h"
#include "common/lrit/lrit_file.h"
#include "image/image.h"
#include "msg_headers.h"
#include "xrit/identify.h"
#include "xrit/processor/get_img.h"

namespace satdump
{
    namespace xrit
    {
        class MSGSegmentedImageDecoder : public SegmentedImageDecoder
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

            MSGSegmentedImageDecoder(lrit::LRITFile &file)
            {
                lrit::ImageStructureRecord image_structure_record = file.getHeader<::lrit::ImageStructureRecord>();
                msg::SegmentIdentificationHeader segment_id_header = file.getHeader<msg::SegmentIdentificationHeader>();

                init(image_structure_record.bit_per_pixel > 8 ? 16 : 8,                                   //
                     segment_id_header.planned_end_segment - segment_id_header.planned_start_segment + 1, //
                     image_structure_record.columns_count,                                                //
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
                msg::SegmentIdentificationHeader segment_id_header = file.getHeader<msg::SegmentIdentificationHeader>();
                auto img = getImageFromXRITFile(XRIT_MSG_SEVIRI, file);
                pushSegment(img, segment_id_header.segment_sequence_number - segment_id_header.planned_start_segment);
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
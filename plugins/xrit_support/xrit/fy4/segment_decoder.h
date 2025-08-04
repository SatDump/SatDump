#pragma once

/**
 * @file segment_decoder.h
 * @brief FY-4x Segmented decoder
 */

#include "../segment_decoder.h"
#include "image/image.h"
#include "utils/string.h"
#include "xrit/fy4/fy4_headers.h"
#include "xrit/identify.h"
#include "xrit/processor/get_img.h"
#include "xrit/xrit_file.h"
#include <exception>

namespace satdump
{
    namespace xrit
    {
        /**
         * @brief FY4x-specific Segmented Decoder
         */
        class FY4xSegmentedImageDecoder : public SegmentedImageDecoder
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

            FY4xSegmentedImageDecoder(XRITFile &file)
            {
                fy4::ImageInformationRecord image_structure_record = file.getHeader<fy4::ImageInformationRecord>();

                // logger->critical("INIT %d %d", image_structure_record.columns_count, image_structure_record.lines_count);

                init(image_structure_record.bit_per_pixel > 8 ? 16 : 8, //
                     image_structure_record.total_segment_count,        //
                     image_structure_record.columns_count,              //
                     image_structure_record.lines_count);
            }

            void pushSegment(image::Image &data, int segc, int p)
            {
                if (segc >= seg_count || segc < 0)
                    return;
                // logger->critical("PUSH %d %d %d/%d %d/%d", data.width(), data.height(), p, image.height(), p + data.height(), image.height());
                //  if (data.size() != seg_height * seg_width)
                //  {
                //      logger->error("Image of the wrong size! (%d %d)", data.size(), seg_height * seg_width);
                //      return;
                //  }
                try
                {
                    image::imemcpy(image, /*(seg_height * seg_width)*/ seg_width * p, data, 0, data.size());
                    segments_done[segc] = true;
                }
                catch (std::exception &e)
                {
                    logger->critical("Weird FY-4x stuff? %s", e.what());
                }
            }

            void pushSegment(XRITFile &file)
            {
                auto img = getImageFromXRITFile(XRIT_FY4_AGRI, file);

                fy4::ImageInformationRecord image_structure_record = file.getHeader<fy4::ImageInformationRecord>();

                pushSegment(img, image_structure_record.current_segment_pos - 1, image_structure_record.current_segment_line_pos);
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
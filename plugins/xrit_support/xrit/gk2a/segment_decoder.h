#pragma once

/**
 * @file segment_decoder.h
 * @brief GK-2A Segmented decoder
 */

#include "../segment_decoder.h"
#include "image/image.h"
#include "utils/string.h"
#include "xrit/gk2a/gk2a_headers.h"
#include "xrit/identify.h"
#include "xrit/processor/get_img.h"
#include "xrit/xrit_file.h"

namespace satdump
{
    namespace xrit
    {
        /**
         * @brief GK-2A-specific Segmented Decoder
         */
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
                seg_height = segment_width; // segment_height; TODOREWORKXRIT check we ONLY get FD?
                seg_width = segment_width;

                image.fill(0);
            }

            GK2ASegmentedImageDecoder(XRITFile &file)
            {
                ImageStructureRecord image_structure_record = file.getHeader<ImageStructureRecord>();

                init(image_structure_record.bit_per_pixel > 8 ? 16 : 8,                                                                                        //
                     file.hasHeader<gk2a::ImageSegmentationIdentification>() ? file.getHeader<gk2a::ImageSegmentationIdentification>().total_segments_nb : 10, //
                     image_structure_record.columns_count,                                                                                                     //
                     image_structure_record.lines_count);
            }

            void pushSegment(image::Image &data, int segc, int pos)
            {
                if (segc >= seg_count || segc < 0 || pos < 0)
                    return;
                if (data.height() + pos > image.height() || data.width() != seg_width)
                {
                    logger->error("Image of the wrong size! %dx%d | %dx%d | %d/%d", seg_width, seg_height, data.width(), data.height(), data.height() + pos, image.height());
                    return;
                }
                image::imemcpy(image, seg_width * pos, data, 0, data.size());
                segments_done[segc] = true;
            }

            void pushSegment(XRITFile &file)
            {
                auto img = getImageFromXRITFile(XRIT_GK2A_AMI, file);

                std::vector<std::string> header_parts = splitString(file.filename, '_');

                int seg_number = 0;
                if (header_parts.size() >= 7)
                {
                    seg_number = std::stoi(header_parts[6].substr(0, header_parts.size() - 4)) - 1;
                }
                else
                {
                    logger->critical("Could not parse segment number from filename!");
                    return;
                }

                if (file.hasHeader<gk2a::ImageSegmentationIdentification>())
                    pushSegment(img, seg_number, file.getHeader<gk2a::ImageSegmentationIdentification>().line_nb - 1);
                else
                    logger->error("Segment header missing!");
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
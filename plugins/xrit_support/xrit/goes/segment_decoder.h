#pragma once

/**
 * @file segment_decoder.h
 * @brief GOES Segmented decoder
 */

#include "image/image.h"
#include "logger.h"
#include "xrit/goes/goes_headers.h"
#include "xrit/identify.h"
#include "xrit/segment_decoder.h"
#include "xrit/xrit_file.h"
#include <cstring>

namespace satdump
{
    namespace xrit
    {
        /**
         * @brief GOES-specific Segmented Decoder
         */
        class GOESSegmentedImageDecoder : public SegmentedImageDecoder
        {
        private:
            int seg_count = 0;
            std::vector<bool> segments_done;
            int seg_size = 0;

        public:
            void init(int max_seg, int max_width, int max_height)
            {
                seg_count = max_seg;
                segments_done = std::vector<bool>(seg_count, false);
                image.init(8, max_width, max_height, 1);
                seg_size = int(max_height / max_seg) * max_width;
            }

            GOESSegmentedImageDecoder(XRITFile &file)
            {
                goes::SegmentIdentificationHeader segment_id_header = file.getHeader<goes::SegmentIdentificationHeader>();

                if (segment_id_header.max_row > 0)
                    init(segment_id_header.max_segment, segment_id_header.max_column, segment_id_header.max_row);
                else
                {
                    ImageStructureRecord image_structure_record = file.getHeader<ImageStructureRecord>();
                    init(segment_id_header.max_segment, segment_id_header.max_column, segment_id_header.max_segment * image_structure_record.lines_count);
                }
            }

            void pushSegment(uint8_t *data, size_t this_size, int segc)
            {
                if (segc >= seg_count || segc < 0)
                    return;
                if (seg_size * segc + this_size <= image.size())
                    std::memcpy((uint8_t *)image.raw_data() + seg_size * segc, data, this_size); // IMGTODO, maybe check 8-bits?
                else
                    logger->error("TRIED TO WRITE OUT OF BOUNDS!");
                segments_done[segc] = true;
            }

            void pushSegment(XRITFile &file)
            {
                PrimaryHeader primary_header = file.getHeader<PrimaryHeader>();
                goes::SegmentIdentificationHeader segment_id_header = file.getHeader<goes::SegmentIdentificationHeader>();
                goes::NOAALRITHeader noaa_header = file.getHeader<goes::NOAALRITHeader>();
                ImageStructureRecord image_structure_record = file.getHeader<ImageStructureRecord>();

                if (file.lrit_data.size() - primary_header.total_header_length < image_structure_record.columns_count * image_structure_record.lines_count)
                {
                    logger->info("Image is too small compared to its header!");
                    return;
                }

                // GOES-N and Himawari have offset segments
                if (noaa_header.product_id == goes::ID_HIMAWARI || (noaa_header.product_id <= 15 && noaa_header.product_id >= 13))
                    pushSegment(&file.lrit_data[primary_header.total_header_length], file.lrit_data.size() - primary_header.total_header_length, segment_id_header.segment_sequence_number - 1);
                else // The rest is normal
                    pushSegment(&file.lrit_data[primary_header.total_header_length], file.lrit_data.size() - primary_header.total_header_length, segment_id_header.segment_sequence_number);
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
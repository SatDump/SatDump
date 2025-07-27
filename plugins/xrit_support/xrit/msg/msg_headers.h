#pragma once

#include "common/utils.h"
#include <cstdint>
#include <cstring>
#include <vector>

namespace satdump
{
    namespace xrit
    {
        namespace msg
        {
            enum CustomFileParams
            {
                JPEG_COMPRESSED,
                WT_COMPRESSED,
            };

            struct SegmentIdentificationHeader
            {
                static constexpr int TYPE = 128;

                uint8_t type;
                uint16_t record_length;
                uint16_t sc_id;
                uint8_t channel_id;
                uint16_t segment_sequence_number;
                uint16_t planned_start_segment;
                uint16_t planned_end_segment;
                uint8_t compression;

                SegmentIdentificationHeader(uint8_t *data)
                {
                    type = data[0];
                    record_length = data[1] << 8 | data[2];
                    sc_id = data[3] << 8 | data[4];
                    channel_id = data[5];
                    segment_sequence_number = data[6] << 8 | data[7];
                    planned_start_segment = data[8] << 8 | data[9];
                    planned_end_segment = data[10] << 8 | data[11];
                    compression = data[12];
                }
            };
        } // namespace msg
    } // namespace xrit
} // namespace satdump
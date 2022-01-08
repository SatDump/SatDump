#pragma once

#include <cstdint>

namespace goes
{
    namespace grb
    {
        struct GRBSecondaryHeader
        {
            uint16_t day_since_epoch;
            uint32_t ms_of_day;
            uint8_t grb_version;
            uint8_t grb_payload_variant;
            uint8_t assembler_identifier;
            uint8_t system_environment;

            GRBSecondaryHeader()
            {
            }

            GRBSecondaryHeader(uint8_t *data)
            {
                day_since_epoch = data[0] << 8 | data[1];
                ms_of_day = data[2] << 24 | data[3] << 16 | data[4] << 8 | data[5];
                grb_version = data[6] >> 3;
                grb_payload_variant = (data[6] & 0b111) << 2 | data[7] >> 6;
                assembler_identifier = (data[7] >> 4) & 0b11;
                system_environment = data[7] & 0b1111;
            }
        };

        enum GRBPayloadVariants
        {
            GENERIC = 0,
            RESERVED = 1,
            IMAGE = 2,
            IMAGE_WITH_DQF = 3,
        };

        struct GRBImagePayloadHeader
        {
            uint8_t compression_algorithm;
            uint32_t seconds_since_epoch;
            uint32_t microsecond_of_second;
            uint16_t block_sequence_count;
            uint32_t row_offset_image_block;
            uint32_t left_x_coord;
            uint32_t left_y_coord;
            uint32_t image_block_height;
            uint32_t image_block_width;
            uint32_t byte_offset_dqf;

            double utc_time;

            GRBImagePayloadHeader(uint8_t *data)
            {
                compression_algorithm = data[0];
                seconds_since_epoch = data[1] << 24 | data[2] << 16 | data[3] << 8 | data[4];
                microsecond_of_second = data[5] << 24 | data[6] << 16 | data[7] << 8 | data[8];
                block_sequence_count = data[9] << 8 | data[10];
                row_offset_image_block = data[11] << 16 | data[12] << 8 | data[13];
                left_x_coord = data[14] << 24 | data[15] << 16 | data[16] << 8 | data[17];
                left_y_coord = data[18] << 24 | data[19] << 16 | data[20] << 8 | data[21];
                image_block_height = data[22] << 24 | data[23] << 16 | data[24] << 8 | data[25];
                image_block_width = data[26] << 24 | data[27] << 16 | data[28] << 8 | data[29];
                byte_offset_dqf = data[30] << 24 | data[31] << 16 | data[32] << 8 | data[33];

                utc_time = (4383 * 3600 * 24) + seconds_since_epoch + double(microsecond_of_second) / 1000.0f;
            }
        };

        struct GRBGenericPayloadHeader
        {
            uint8_t compression_algorithm;
            uint32_t seconds_since_epoch;
            uint32_t microsecond_of_second;
            uint64_t reserved;
            uint32_t data_unit_sequence_count;

            double utc_time;

            GRBGenericPayloadHeader(uint8_t *data)
            {
                compression_algorithm = data[0];
                seconds_since_epoch = data[1] << 24 | data[2] << 16 | data[3] << 8 | data[4];
                microsecond_of_second = data[5] << 24 | data[6] << 16 | data[7] << 8 | data[8];
                // Reserved, 8 bytes
                data_unit_sequence_count = data[16] << 24 | data[17] << 16 | data[18] << 8 | data[19];

                utc_time = (4383 * 3600 * 24) + seconds_since_epoch + double(microsecond_of_second) / 1000.0f;
            }
        };

        enum GRBImageCompressionAlgo
        {
            NO_COMPRESSION = 0,
            JPEG_2000 = 1,
            SZIP = 2,
        };
    }
}
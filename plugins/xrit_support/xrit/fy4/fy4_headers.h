#pragma once

/**
 * @file fy4_headers.h
 * @brief FY-4x HRIT/LRIT Specific Headers
 */

#include "common/utils.h"
#include <cstdint>
#include <cstring>
#include <vector>

namespace satdump
{
    namespace xrit
    {
        namespace fy4
        {
            struct KeyHeader
            {
                static constexpr int TYPE = 7;

                uint8_t type;
                uint16_t record_length;
                uint32_t key;

                KeyHeader(uint8_t *data)
                {
                    type = data[0];
                    record_length = data[1] << 8 | data[2];
                    key = data[3] << 24 | data[4] << 16 | data[5] << 8 | data[6];
                }
            };

            struct ImageInformationRecord
            {
                static constexpr int TYPE = 1;

                uint8_t type;
                uint16_t record_length;

                std::string satellite_name;
                std::string instrument_name;

                uint8_t bit_per_pixel;
                uint16_t columns_count;
                uint16_t lines_count;
                uint8_t compression_flag;

                uint8_t channel_number;
                uint8_t total_segment_count;
                uint8_t current_segment_number;
                uint8_t current_segment_pos;
                uint16_t current_segment_line_pos;

                uint8_t compressed_info_algo;
                uint8_t compressed_info_lossless;
                uint8_t compressed_info_level;

                ImageInformationRecord(uint8_t *data)
                {
                    type = data[0];
                    record_length = data[1] << 8 | data[2];
                    satellite_name = std::string(&data[3], &data[3 + 9]);
                    instrument_name = std::string(&data[12], &data[12 + 7]);
                    bit_per_pixel = data[19];
                    columns_count = data[20] << 8 | data[21];
                    lines_count = data[22] << 8 | data[23];
                    compression_flag = data[24];

                    channel_number = data[25];
                    total_segment_count = data[26];
                    current_segment_number = data[27];
                    current_segment_pos = data[28];
                    current_segment_line_pos = data[29] << 8 | data[30];

                    compressed_info_algo = data[31] >> 7;
                    compressed_info_lossless = (data[31] >> 6) & 1;
                    compressed_info_level = data[31] & 0b111111;
                }
            };
        } // namespace fy4
    } // namespace xrit
} // namespace satdump
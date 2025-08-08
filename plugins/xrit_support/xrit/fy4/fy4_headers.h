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

            struct ImageNavigationRecord
            {
                static constexpr int TYPE = 2;

                uint8_t type;
                uint16_t record_length;
                std::string projection_name;
                float column_scaling_factor;
                float line_scaling_factor;
                float column_offset;
                float line_offset;

                // double nominal_point_longitude;
                // double sub_satellite_longitude;
                // uint8_t projection_type;
                // std::string epoch_time_mjd;
                // double semi_major_axis;
                // double eccentricity;
                // double inclination;
                // double ascending_node_longitude;
                // double perigee_angle1;
                // double perigee_angle2;
                // TODOREWORK ALL THE REST

                // Pre-computed projection information
                double column_scalar;
                double line_scalar;

                ImageNavigationRecord(uint8_t *data)
                {
                    type = data[0];
                    record_length = data[1] << 8 | data[2];
                    projection_name = std::string((char *)&data[3], (char *)&data[3 + 32]);

                    ((float *)&column_scaling_factor)[3] = data[35];
                    ((float *)&column_scaling_factor)[2] = data[36];
                    ((float *)&column_scaling_factor)[1] = data[37];
                    ((float *)&column_scaling_factor)[0] = data[38];

                    ((float *)&line_scaling_factor)[3] = data[39];
                    ((float *)&line_scaling_factor)[2] = data[40];
                    ((float *)&line_scaling_factor)[1] = data[41];
                    ((float *)&line_scaling_factor)[0] = data[42];

                    ((float *)&column_offset)[3] = data[43];
                    ((float *)&column_offset)[2] = data[44];
                    ((float *)&column_offset)[1] = data[45];
                    ((float *)&column_offset)[0] = data[46];

                    ((float *)&line_offset)[3] = data[47];
                    ((float *)&line_offset)[2] = data[48];
                    ((float *)&line_offset)[1] = data[49];
                    ((float *)&line_offset)[0] = data[50];

                    column_scalar = line_scalar = 0.0;
                }
            };
        } // namespace fy4
    } // namespace xrit
} // namespace satdump
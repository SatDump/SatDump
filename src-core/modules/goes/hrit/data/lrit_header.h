#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

namespace goes
{
    namespace hrit
    {
        struct PrimaryHeader
        {
            static constexpr int TYPE = 0;

            uint8_t type;
            uint16_t record_length;
            uint8_t file_type_code;
            uint32_t total_header_length;
            uint64_t data_field_length;

            PrimaryHeader(uint8_t *data)
            {
                type = data[0];
                record_length = data[1] << 8 | data[2];
                file_type_code = data[3];
                total_header_length = (uint32_t)data[4] << 24 |
                                      (uint32_t)data[5] << 16 |
                                      (uint32_t)data[6] << 8 |
                                      (uint32_t)data[7];
                data_field_length = (uint64_t)data[8] << 56 |
                                    (uint64_t)data[9] << 48 |
                                    (uint64_t)data[10] << 40 |
                                    (uint64_t)data[11] << 32 |
                                    (uint64_t)data[12] << 24 |
                                    (uint64_t)data[13] << 16 |
                                    (uint64_t)data[14] << 8 |
                                    (uint64_t)data[15];
            }
        };

        struct ImageStructureRecord
        {
            static constexpr int TYPE = 1;

            uint8_t type;
            uint16_t record_length;
            uint8_t bit_per_pixel;
            uint16_t columns_count;
            uint16_t lines_count;
            uint8_t compression_flag;

            ImageStructureRecord(uint8_t *data)
            {
                type = data[0];
                record_length = data[1] << 8 | data[2];
                bit_per_pixel = data[3];
                columns_count = data[4] << 8 | data[5];
                lines_count = data[6] << 8 | data[7];
                compression_flag = data[8];
            }
        };

        struct ImageNavigationRecord
        {
            static constexpr int TYPE = 2;

            uint8_t type;
            uint16_t record_length;
            char projection_name[32];
            uint32_t column_scaling_factor;
            uint32_t line_scaling_factor;
            uint32_t column_offset;
            uint32_t line_offset;

            ImageNavigationRecord(uint8_t *data)
            {
                type = data[0];
                record_length = data[1] << 8 | data[2];
                std::memcpy(projection_name, &data[3], 32);
                column_scaling_factor = (uint32_t)data[36] << 24 |
                                        (uint32_t)data[37] << 16 |
                                        (uint32_t)data[38] << 8 |
                                        (uint32_t)data[39];
                line_scaling_factor = (uint32_t)data[40] << 24 |
                                      (uint32_t)data[41] << 16 |
                                      (uint32_t)data[42] << 8 |
                                      (uint32_t)data[43];
                column_offset = (uint32_t)data[44] << 24 |
                                (uint32_t)data[45] << 16 |
                                (uint32_t)data[46] << 8 |
                                (uint32_t)data[47];
                line_offset = (uint32_t)data[48] << 24 |
                              (uint32_t)data[49] << 16 |
                              (uint32_t)data[50] << 8 |
                              (uint32_t)data[51];
            }
        };

        struct ImageDataFunctionRecord
        {
            static constexpr int TYPE = 3;

            uint8_t type;
            uint16_t record_length;
            std::vector<char> data_definition;

            ImageDataFunctionRecord(uint8_t *data)
            {
                type = data[0];
                record_length = data[1] << 8 | data[2];
                data_definition.insert(data_definition.end(), &data[3], &data[record_length]);
            }
        };

        struct AnnotationRecord
        {
            static constexpr int TYPE = 4;

            uint8_t type;
            uint16_t record_length;
            std::vector<char> annotation_text;

            AnnotationRecord(uint8_t *data)
            {
                type = data[0];
                record_length = data[1] << 8 | data[2];
                annotation_text.insert(annotation_text.end(), &data[3], &data[record_length]);
            }
        };

        struct TimeStampRecord
        {
            static constexpr int TYPE = 5;

            uint8_t type;
            uint16_t record_length;
            // TODO

            TimeStampRecord(uint8_t *data)
            {
                type = data[0];
                record_length = data[1] << 8 | data[2];
                //ancillary_text.insert(ancillary_text.end(), &data[3], &data[record_length]);
            }
        };

        struct AncillaryTextRecord
        {
            static constexpr int TYPE = 6;

            uint8_t type;
            uint16_t record_length;
            std::vector<char> ancillary_text;

            AncillaryTextRecord(uint8_t *data)
            {
                type = data[0];
                record_length = data[1] << 8 | data[2];
                ancillary_text.insert(ancillary_text.end(), &data[3], &data[record_length]);
            }
        };

        struct KeyHeader
        {
            static constexpr int TYPE = 7;

            uint8_t type;
            uint16_t record_length;
        };

        struct SegmentIdentificationHeader
        {
            static constexpr int TYPE = 128;

            uint8_t type;
            uint16_t record_length;
            uint16_t image_identifier;
            uint16_t segment_sequence_number;
            uint16_t start_column_of_segment;
            uint16_t start_line_of_segment;
            uint16_t max_segment;
            uint16_t max_column;
            uint16_t max_row;

            SegmentIdentificationHeader(uint8_t *data)
            {
                type = data[0];
                record_length = data[1] << 8 | data[2];
                image_identifier = data[3] << 8 | data[4];
                segment_sequence_number = data[5] << 8 | data[6];
                start_column_of_segment = data[7] << 8 | data[8];
                start_line_of_segment = data[9] << 8 | data[10];
                max_segment = data[11] << 8 | data[12];
                max_column = data[13] << 8 | data[14];
                max_row = data[15] << 8 | data[16];
            }
        };

        struct NOAALRITHeader
        {
            static constexpr int TYPE = 129;

            uint8_t type;
            uint16_t record_length;
            char agency_signature[4];
            uint16_t product_id;
            uint16_t product_subid;
            uint16_t parameter;
            uint8_t noaa_specific_compression;

            NOAALRITHeader(uint8_t *data)
            {
                type = data[0];
                record_length = data[1] << 8 | data[2];
                std::memcpy(agency_signature, &data[3], 4);
                product_id = data[7] << 8 | data[8];
                product_subid = data[9] << 8 | data[10];
                parameter = data[11] << 8 | data[12];
                noaa_specific_compression = data[13];
            }
        };

        struct HeaderStructureRecord
        {
            static constexpr int TYPE = 130;

            uint8_t type;
            uint16_t record_length;
            std::vector<char> header_structure;

            HeaderStructureRecord(uint8_t *data)
            {
                type = data[0];
                record_length = data[1] << 8 | data[2];
                header_structure.insert(header_structure.end(), &data[3], &data[record_length]);
            }
        };

        struct RiceCompressionHeader
        {
            static constexpr int TYPE = 131;

            uint8_t type;
            uint16_t record_length;
            uint16_t flags;
            uint8_t pixels_per_block;
            uint8_t scanlines_per_packet;

            RiceCompressionHeader(uint8_t *data)
            {
                type = data[0];
                record_length = data[1] << 8 | data[2];
                flags = data[3] << 8 | data[4];
                pixels_per_block = data[5];
                scanlines_per_packet = data[6];
            }
        };
    }
}
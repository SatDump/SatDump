#pragma once

/**
 * @file goes_headers.h
 * @brief GOES HRIT/LRIT Specific Headers
 */

#include "utils/string.h"
#include <cstdint>
#include <cstring>
#include <map>
#include <vector>

namespace satdump
{
    namespace xrit
    {
        namespace goes
        {
            const int ID_HIMAWARI = 43;

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

            struct AncillaryTextRecord
            {
                static constexpr int TYPE = 6;

                uint8_t type;
                uint16_t record_length;
                std::string ancillary_text;

                std::map<std::string, std::string> meta;

                AncillaryTextRecord(uint8_t *data)
                {
                    type = data[0];
                    record_length = data[1] << 8 | data[2];
                    ancillary_text.insert(ancillary_text.end(), &data[3], &data[record_length]);

                    // I will admit I needed to peek in goestools to figure that one out
                    // A bit hard without having live data...
                    std::vector<std::string> fields = splitString(ancillary_text, ';');

                    for (std::string &field : fields)
                    {
                        std::vector<std::string> values = splitString(field, '=');
                        if (values.size() == 2)
                        {
                            values[0] = values[0].substr(0, values[0].find_last_not_of(' ') + 1);
                            values[1] = values[1].substr(values[1].find_first_not_of(' '));
                            meta.insert({values[0], values[1]});
                        }
                    }
                }
            };

            struct AnnotationRecord
            {
                static constexpr int TYPE = 4;

                uint8_t type;
                uint16_t record_length;
                std::string annotation_text;

                AnnotationRecord(uint8_t *data)
                {
                    type = data[0];
                    record_length = data[1] << 8 | data[2];
                    annotation_text.insert(annotation_text.end(), &data[3], &data[record_length]);
                }
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
        } // namespace goes
    } // namespace xrit
} // namespace satdump
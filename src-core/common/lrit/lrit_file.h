#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include "libs/others/strptime.h"
#include <ctime>

namespace lrit
{
    ////////////////////////////////////////
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
        std::string projection_name;
        int32_t column_scaling_factor;
        int32_t line_scaling_factor;
        int32_t column_offset;
        int32_t line_offset;

        ImageNavigationRecord(uint8_t *data)
        {
            type = data[0];
            record_length = data[1] << 8 | data[2];
            projection_name = std::string((char *)&data[3], (char *)&data[3 + 32]);
            column_scaling_factor = data[35] << 24 | data[36] << 16 | data[37] << 8 | data[38];
            line_scaling_factor = data[39] << 24 | data[40] << 16 | data[41] << 8 | data[42];
            column_offset = data[43] << 24 | data[44] << 16 | data[45] << 8 | data[46];
            line_offset = data[47] << 24 | data[48] << 16 | data[49] << 8 | data[50];
            //   column_scaling_factor = __builtin_bswap32(*reinterpret_cast<const uint32_t *>(&data[35]));
            //   line_scaling_factor = __builtin_bswap32(*reinterpret_cast<const uint32_t *>(&data[39]));
            //   column_offset = __builtin_bswap32(*reinterpret_cast<const uint32_t *>(&data[43]));
            //   line_offset = __builtin_bswap32(*reinterpret_cast<const uint32_t *>(&data[47]));
        }
    };

    struct ImageDataFunctionRecord
    {
        static constexpr int TYPE = 3;

        uint8_t type;
        uint16_t record_length;
        std::string datas;

        ImageDataFunctionRecord(uint8_t *data)
        {
            type = data[0];
            record_length = data[1] << 8 | data[2];
            datas = std::string((char *)&data[3], (char *)&data[3 + record_length - 3]);
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

    struct TimeStampRecord
    {
        static constexpr int TYPE = 5;

        uint8_t type;
        uint16_t record_length;
        uint16_t days;
        uint32_t milliseconds_of_day;

        time_t timestamp;

        TimeStampRecord(uint8_t *data)
        {
            type = data[0];
            record_length = data[1] << 8 | data[2];
            uint16_t days = data[3] << 8 | data[4];
            uint32_t milliseconds_of_day = data[5] << 24 | data[6] << 16 | data[7] << 8 | data[8];

            timestamp = (days - 4383) * 86400 + milliseconds_of_day;
        }
    };
    ////////////////////////////////////////

    struct LRITFile
    {
        int vcid = -1;

        bool file_in_progress = false;
        bool header_parsed = false;

        std::map<int, int> custom_flags;

        std::string filename;
        int total_header_length;
        std::map<int, int> all_headers;
        std::vector<uint8_t> lrit_data;

        template <typename T>
        T getHeader()
        {
            if constexpr (std::is_same_v<T, PrimaryHeader>)
                return PrimaryHeader(&lrit_data[0]);
            else
                return T(&lrit_data[all_headers[T::TYPE]]);
        }

        template <typename T>
        bool hasHeader()
        {
            return all_headers.count(T::TYPE);
        }

        void parseHeaders();
    };
};
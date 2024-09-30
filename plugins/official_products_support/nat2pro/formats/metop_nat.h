#pragma once

#include <cstdint>

namespace nat2pro
{
    inline double get_deg_latlon(uint8_t *data)
    {
        int32_t val;
        ((uint8_t *)&val)[3] = data[0];
        ((uint8_t *)&val)[2] = data[1];
        ((uint8_t *)&val)[1] = data[2];
        ((uint8_t *)&val)[0] = data[3];
        return double(val) / 1e4;
    }

    struct GenericRecordHeader
    {
        uint8_t record_class;
        uint8_t instrument_group;
        uint8_t record_subclass;
        uint8_t record_subclass_version;
        uint32_t record_size;
        double record_start_time;
        double record_stop_time;

        GenericRecordHeader(uint8_t *data)
        {
            record_class = data[0];
            instrument_group = data[1];
            record_subclass = data[2];
            record_subclass_version = data[3];
            record_size = data[4] << 24 | data[5] << 16 | data[6] << 8 | data[7];
            // record_start_time = data[8] << 40 | data[9] << 32 | data[10] << 24 | data[11] << 16 | data[12] << 8 | data[13];
            // record_stop_time = data[14] << 40 | data[15] << 32 | data[16] << 24 | data[17] << 16 | data[18] << 8 | data[19];
        }
    };
}
#pragma once

#include <cstdint>

enum gvar_product_id
{
    NO_DATA,
    AAA_IR_DATA,
    AAA_VISIBLE_DATA,
    GVAR_IMAGER_DOCUMENATION,
    GVAR_IMAGER_IR_DATA,
    GVAR_IMAGER_VISIBLE_DATA,
    GVAR_SOUNDER_DOCUMENTATION,
    GVAR_SOUNDER_SCAN_DATA,
    GVAR_COMPENSATION_DATA,
    GVAR_TELEMTRY_STATISTICS,
    GVAR_AUX_TEXT,
    GIMTACTS_TEXT,
    SPS_TEXT,
    AAA_SOUNDING_PRODUCTS,
    GVAR_ECAL_DATA,
    GVAR_SPACELOOK_DATA,
    GVAR_BB_DATA,
    GVAR_CALIBRATION_COEFFICIENTS,
    GVAR_VISIBLE_NLUTS,
    GVAR_STAR_SENSE_DATA,
    IMAGER_FACTORY_COEFFICIENTS
};

#ifdef _WIN32
#pragma pack(push, 1)
#endif
struct bcd_date_t
{
    uint8_t YEAR100:4;
    uint8_t YEAR1000:4;

    uint8_t YEAR1:4;
    uint8_t YEAR10:4;

    uint8_t DOY10:4;
    uint8_t DOY100:4;

    uint8_t HOURS10:4;
    uint8_t DOY1:4;

    uint8_t MINUTES10:4;
    uint8_t HOURS1:4;

    uint8_t SECONDS10:4;
    uint8_t MINUTES1:4;

    uint8_t MSECONDS100:4;
    uint8_t SECONDS1:4;

    uint8_t MSECONDS1:4;
    uint8_t MSECONDS10:4;
};
struct PrimaryBlockHeader
{
    uint8_t block_id;
    uint8_t word_size;
    uint16_t word_count;
    uint16_t product_id;
    uint8_t repeat_flag;
    uint8_t version_number;
    uint8_t data_valid;
    uint8_t ascii_binary;
    uint8_t sps_id;
    uint8_t range_word;
    uint16_t block_count;
    uint16_t spare1;
    bcd_date_t time_code_bcd;
    uint32_t spare2;
    uint16_t header_crc;
}
#ifdef _WIN32
;
#else
__attribute__((packed));
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif

struct LineDocumentationHeader
{
    uint16_t sc_id;
    uint16_t sps_id;
    uint16_t l_side;
    uint16_t detector_number;
    uint16_t source_channel;
    uint32_t relative_scan_count;
    uint16_t imager_scan_status_1;
    uint16_t imager_scan_status_2;
    uint16_t pixel_count;
    uint16_t word_count;
    uint16_t zonal_correction;
    uint16_t llag;
    uint16_t spare;

    LineDocumentationHeader(uint8_t *data)
    {
        uint16_t data_buffer[16];

        int pos = 0;
        for (int i = 0; i < 16; i += 4)
        {
            data_buffer[i] = (data[pos + 0] << 2) | (data[pos + 1] >> 6);
            data_buffer[i + 1] = ((data[pos + 1] % 64) << 4) | (data[pos + 2] >> 4);
            data_buffer[i + 2] = ((data[pos + 2] % 16) << 6) | (data[pos + 3] >> 2);
            data_buffer[i + 3] = ((data[pos + 3] % 4) << 8) | data[pos + 4];
            pos += 5;
        }

        sc_id = data_buffer[0];
        sps_id = data_buffer[1];
        l_side = data_buffer[2];
        detector_number = data_buffer[3];
        source_channel = data_buffer[4];
        relative_scan_count = data_buffer[5] << 10 | data_buffer[6];
        imager_scan_status_1 = data_buffer[7];
        imager_scan_status_2 = data_buffer[8];
        pixel_count = data_buffer[9] << 10 | data_buffer[10];
        word_count = data_buffer[11] << 10 | data_buffer[12];
        zonal_correction = data_buffer[13];
        llag = data_buffer[14];
        spare = data_buffer[15];
    }
};
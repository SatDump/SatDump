#pragma once

#include "common/ccsds/ccsds.h"
#include <vector>
#include "image/image.h"

namespace aim
{
    namespace cips
    {
        class CIPSScienceHeader
        {
        public:
            uint32_t image_exposure_time_seconds;
            uint32_t image_exposure_time_subseconds;
            uint32_t start_pixel;
            uint8_t bits_pixel;
            uint16_t image_columns;
            uint16_t image_rows;
            uint8_t compression_scheme;
            bool software_binning_performed;
            bool camera_binning_performed;
            bool camera_mode;
            uint32_t exposure_time;
            uint16_t tdi_row_count;
            uint8_t packet_version;
            uint16_t crc;

        public:
            CIPSScienceHeader(uint8_t *data)
            {
                image_exposure_time_seconds = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
                image_exposure_time_subseconds = data[4] << 24 | data[5] << 16 | data[6] << 8 | data[7];
                start_pixel = data[8] << 16 | data[9] << 8 | data[10];
                bits_pixel = data[11];
                image_columns = data[12] << 8 | data[13];
                image_rows = data[14] << 8 | data[15];
                compression_scheme = data[16] >> 3;
                software_binning_performed = data[16] & 0b100;
                camera_binning_performed = data[16] & 0b010;
                camera_mode = data[16] & 0b001;
                exposure_time = data[17] << 8 | data[18];
                tdi_row_count = data[19] << 8 | data[20];
                packet_version = data[21];
                crc = data[22] << 8 | data[23];
            }
        };

        class CIPSReader
        {
        private:
            int img_width = 0;
            int img_height = 0;

            int img_c = 0;

            void add_image()
            {
                images.push_back(image::Image(16, img_width, img_height, 1));
            }

        public:
            CIPSReader();
            ~CIPSReader();

            std::vector<image::Image> images;

            void init(int w, int h)
            {
                img_width = w;
                img_height = h;
                add_image();
            }

            void work(ccsds::CCSDSPacket &packet);
        };
    } // namespace swap
} // namespace proba
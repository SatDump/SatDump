#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include <map>
#include "common/image/image.h"

namespace eos
{
    namespace modis
    {
        struct MODISHeader
        {
            MODISHeader(ccsds::CCSDSPacket &pkt)
            {
                day_count = pkt.payload[0] << 8 | pkt.payload[1];
                coarse_time = pkt.payload[2] << 24 | pkt.payload[3] << 16 | pkt.payload[4] << 8 | pkt.payload[5];
                fine_time = pkt.payload[6] << 8 | pkt.payload[7];

                quick_look = pkt.payload[8] >> 7;
                packet_type = (pkt.payload[8] >> 4) % (int)pow(2, 3);
                scan_count = (pkt.payload[8] >> 1) % (int)pow(2, 3);
                mirror_side = pkt.payload[8] % 2;

                type_flag = pkt.payload[9] >> 7;

                // If this is Earth scan data
                earth_frame_data_count = (pkt.payload[9] % (int)pow(2, 7)) << 4 | pkt.payload[10] >> 4;

                // If this is calibration data
                calib_type = (pkt.payload[9] >> 5) & 0b11;
                calib_mode = (pkt.payload[9] >> 3) & 0b11;
                calib_frame_count = ((pkt.payload[9] >> 1) & 1) << 5 | pkt.payload[10] >> 3;
            }

            uint16_t day_count;
            uint32_t coarse_time;
            uint16_t fine_time;

            bool quick_look;
            enum modis_pkt_type
            {
                DAY_GROUP,
                NIGHT_GROUP,
                ENG_GROUP_1,
                ENG_GROUP_2
            };
            uint8_t packet_type;
            uint8_t scan_count;
            bool mirror_side;

            bool type_flag;

            uint16_t earth_frame_data_count;

            enum modis_calib_type
            {
                SOLAR_DIFFUSER_SOURCE,
                SRCA_CALIB_SOURCE,
                BLACKBODY_SOURCE,
                SPACE_SOURCE,
            };
            uint8_t calib_type;
            uint8_t calib_mode;
            uint8_t calib_frame_count;
        };

        class MODISReader
        {
        private:
            int lastScanCount;
            unsigned short *channels1000m[31];
            unsigned short *channels500m[5];
            unsigned short *channels250m[2];
            void processDayPacket(ccsds::CCSDSPacket &packet, MODISHeader &header);
            void processNightPacket(ccsds::CCSDSPacket &packet, MODISHeader &header);

        public:
            MODISReader();
            ~MODISReader();
            int day_count, night_count, lines;
            std::vector<double> timestamps_1000;
            std::vector<double> timestamps_500;
            std::vector<double> timestamps_250;
            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint16_t> getImage250m(int channel);
            image::Image<uint16_t> getImage500m(int channel);
            image::Image<uint16_t> getImage1000m(int channel);

            uint16_t common_day;
            uint32_t common_coarse;
        };
    } // namespace modis
} // namespace eos
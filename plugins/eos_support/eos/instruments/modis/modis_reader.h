#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include <vector>
#include "common/image/image.h"
#include "nlohmann/json.hpp"

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
                DAY_GROUP = 0,
                NIGHT_GROUP = 1,
                ENG_GROUP_1 = 2,
                ENG_GROUP_2 = 4,
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
            uint16_t modis_ifov[416]; // All pixels + CRC
            int lastScanCount;
            std::vector<uint16_t> channels1000m[31];
            std::vector<uint16_t> channels500m[5];
            std::vector<uint16_t> channels250m[2];
            void processDayPacket(ccsds::CCSDSPacket &packet, MODISHeader &header);
            void processNightPacket(ccsds::CCSDSPacket &packet, MODISHeader &header);

            void processEng1Packet(ccsds::CCSDSPacket &packet, MODISHeader &header);
            void processEng2Packet(ccsds::CCSDSPacket &packet, MODISHeader &header);

            uint16_t compute_crc(uint16_t *data, int size);

            nlohmann::json d_calib;
            void fillCalib(ccsds::CCSDSPacket &packet, MODISHeader &header);

            uint16_t last_obc_bb_temp[12] = {0};
            uint16_t last_rct_mir_temp[2] = {0};
            uint16_t last_ao_inst_temp[4] = {0};
            uint16_t last_cav_temp[4] = {0};
            uint16_t last_fp_temp[4] = {0};
            bool last_cr_rc_info[4] = {false};

        public:
            MODISReader();
            ~MODISReader();
            int day_count, night_count, lines;
            std::vector<double> timestamps_1000;
            std::vector<double> timestamps_500;
            std::vector<double> timestamps_250;
            void work(ccsds::CCSDSPacket &packet);
            image::Image getImage250m(int channel);
            image::Image getImage500m(int channel);
            image::Image getImage1000m(int channel);
            nlohmann::json getCalib();
        };
    } // namespace modis
} // namespace eos
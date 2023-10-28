#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include "common/image/image.h"

#include "atms_structs.h"

namespace jpss
{
    namespace atms
    {
        class ATMSReader
        {
        private:
            int scan_pos;
            std::vector<uint16_t> channels[22];
            std::vector<uint16_t> channels_cc[22];
            std::vector<uint16_t> channels_wc[22];

            ATMSCalibPkt last_calib_pkt;
            ATMSHealtStatusPkt last_eng_pkt;
            ATMSHotCalTempPkt last_hot_pkt;
            nlohmann::json calib_data;

        public:
            ATMSReader();
            ~ATMSReader();

            int lines;
            std::vector<double> timestamps;

            void work(ccsds::CCSDSPacket &packet);
            void work_calib(ccsds::CCSDSPacket &packet);
            void work_eng(ccsds::CCSDSPacket &packet);
            void work_hotcal(ccsds::CCSDSPacket &packet);

            image::Image<uint16_t> getChannel(int channel);
            nlohmann::json getCalib();
        };
    } // namespace atms
} // namespace jpss
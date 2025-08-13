#pragma once

#include "common/ccsds/ccsds.h"
#include "image/image.h"
#include "nlohmann/json.hpp"

namespace aws
{
    namespace mwr
    {
        class MWRReader
        {
        private:
            std::vector<uint16_t> channels[19];
            std::vector<uint8_t> wip_full_pkt;

            std::vector<std::array<float, 4>> cal_load_temps;
            std::vector<std::array<std::array<uint16_t, 15>, 19>> cal_load_views;
            std::vector<std::array<std::array<uint16_t, 25>, 19>> cal_cold_views;

            void parseCal();

        public:
            MWRReader();
            ~MWRReader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            image::Image getChannel(int channel);
            nlohmann::json getCal()
            {
                nlohmann::json c;
                c["temps"] = cal_load_temps;
                c["load_views"] = cal_load_views;
                c["cold_views"] = cal_cold_views;
                return c;
            }
        };
    } // namespace mwr
} // namespace aws
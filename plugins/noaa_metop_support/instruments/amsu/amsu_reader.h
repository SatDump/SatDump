#pragma once

#include <cstdint>
#include <vector>

#include "common/image/image.h"
#include "common/ccsds/ccsds.h"
#include "common/simple_deframer.h"
#include "nlohmann/json.hpp"

namespace noaa_metop
{
    namespace amsu
    {
        class AMSUReader
        {
        public:
            std::vector<double> timestamps_A1;
            std::vector<double> timestamps_A2;
            double last_TIP_timestamp;
            int linesA1 = 0, linesA2 = 0;

        private:
            struct view_pair
            {
                uint16_t blackbody;
                uint16_t space;
            };
            std::vector<uint16_t> channels[15];
            def::SimpleDeframer amsuA1Deframer = def::SimpleDeframer(0xFFFFFF, 24, 9920, 0, true);
            def::SimpleDeframer amsuA2Deframer = def::SimpleDeframer(0xFFFFFF, 24, 2496, 0, true);
            std::vector<std::vector<uint8_t>> amsuA2Data;
            std::vector<std::vector<uint8_t>> amsuA1Data;
            std::vector<view_pair> calibration_views_A1[13];
            std::vector<view_pair> calibration_views_A2[2];
            std::vector<uint16_t> temperature_counts_A1[45];
            std::vector<uint16_t> temperature_counts_A2[19];

            // calib stuff
            nlohmann::json calib;

        public:
            AMSUReader();
            ~AMSUReader();
            void work_noaa(uint8_t *buffer);
            void work_metop(ccsds::CCSDSPacket &packet);
            image::Image getChannel(int channel)
            {
                image::Image img(channels[channel].data(), 16, 30, (channel < 2 ? linesA2 : linesA1), 1);
                img.mirror(true, false);
                return img;
            }
            void calibrate(nlohmann::json coefs);

            nlohmann::json calib_out;

        private:
            void work_A1(uint8_t *buffer);
            void work_A2(uint8_t *buffer);

            bool contains(std::vector<double> tm, double n)
            {
                for (unsigned int i = 0; i < tm.size(); i++)
                {
                    if (tm[i] == n)
                        return true;
                }
                return false;
            }

            double extrapolate(std::pair<double, double> A, std::pair<double, double> B, std::pair<double, double> C, double x)
            {
                if (x < B.first)
                {
                    return (B.second - A.second) / (B.first - A.second) * (x - A.first) + A.second;
                }
                else
                {
                    return (C.second - B.second) / (C.first - B.second) * (x - B.first) + B.second;
                }
            }
        };
    }; // namespace amsu
};     // namespace noaa_metop
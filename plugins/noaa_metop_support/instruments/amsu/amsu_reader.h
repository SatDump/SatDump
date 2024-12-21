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
            //  std::vector<double> timestamps_A1;
            //  std::vector<double> timestamps_A2;
            double last_TIP_timestamp;
            int linesA1 = 0, linesA2 = 0;

        private:
            struct view_pair
            {
                uint16_t blackbody;
                uint16_t space;
            };

            struct a1_data_t
            {
                uint16_t channels[13][30];
                double timestamp = -1;
                view_pair calibration_views_A1[13];
                uint16_t temperature_counts_A1[45];

                a1_data_t()
                {
                    memset(calibration_views_A1, 0, sizeof(calibration_views_A1));
                    memset(temperature_counts_A1, 0, sizeof(temperature_counts_A1));
                    memset(channels, 0, sizeof(uint16_t) * 13 * 30);
                }
            };

            struct a2_data_t
            {
                uint16_t channels[2][30];
                double timestamp = -1;
                view_pair calibration_views_A2[2];
                uint16_t temperature_counts_A2[19];

                a2_data_t()
                {
                    memset(calibration_views_A2, 0, sizeof(calibration_views_A2));
                    memset(temperature_counts_A2, 0, sizeof(temperature_counts_A2));
                    memset(channels, 0, sizeof(uint16_t) * 2 * 30);
                }
            };

            // std::vector<uint16_t> channels[15];
            def::SimpleDeframer amsuA1Deframer = def::SimpleDeframer(0xFFFFFF, 24, 9920, 0, true);
            def::SimpleDeframer amsuA2Deframer = def::SimpleDeframer(0xFFFFFF, 24, 2496, 0, true);
            // std::vector<std::vector<uint8_t>> amsuA2Data;
            // std::vector<std::vector<uint8_t>> amsuA1Data;
            // std::vector<view_pair> calibration_views_A1[13];
            // std::vector<view_pair> calibration_views_A2[2];
            // std::vector<uint16_t> temperature_counts_A1[45];
            // std::vector<uint16_t> temperature_counts_A2[19];
            std::vector<a1_data_t> amsu_a1_dat;
            std::vector<a2_data_t> amsu_a2_dat;

            // calib stuff
            nlohmann::json calib;

        public:
            AMSUReader();
            ~AMSUReader();
            void work_noaa(uint8_t *buffer);
            void work_metop(ccsds::CCSDSPacket &packet);
            image::Image getChannel(int channel);
            void calibrate(nlohmann::json coefs);

            std::vector<double> common_timestamps;
            void correlate();

            nlohmann::json calib_out;

        private:
            void work_A1(uint8_t *buffer);
            void work_A2(uint8_t *buffer);

            template <typename T>
            bool contains(std::vector<T> tm, double n)
            {
                for (unsigned int i = 0; i < tm.size(); i++)
                {
                    if (tm[i].timestamp == n)
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
}; // namespace noaa_metop
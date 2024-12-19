#pragma once

#include <cstdint>
#include "common/image/image.h"
#include "../../tip_time_parser.h"
#include "../../contains.h"
#include "nlohmann/json.hpp"

namespace noaa
{
    namespace hirs
    {
        struct calib_sequence
        {
        public:
            calib_sequence() {
                for (int i = 0; i < 19; i++){
                    space[i] = 0;
                    blackbody[i] = 0;
                }
            };

            uint16_t position = 0;
            uint16_t space[19];
            uint16_t blackbody[19];
            double PRT_temp = 0;
            bool bb_ready = false, spc_ready = false;

            void calc_space(uint16_t *samples, uint8_t channel)
            {
                space[channel] = calc_avg(samples, 48)>>3;
            }
            void calc_bb(uint16_t *samples, uint8_t channel)
            {
                blackbody[channel] = calc_avg(samples, 56)>>3;
            }
            bool is_ready()
            {
                return bb_ready && spc_ready;
            }

            static uint16_t calc_avg(uint16_t *samples, int count);
        };

        class HIRSReader
        {
        private:
            std::vector<uint16_t> channels[20];
            const int HIRSPositions[36] = {16, 17, 22, 23, 26, 27, 30, 31, 34, 35, 38, 39, 42, 43, 54, 55, 58, 59, 62, 63, 66, 67, 70, 71, 74, 75, 78, 79, 82, 83, 84, 85, 88, 89, 92, 93};
            const int HIRSChannels[20] = {0, 16, 1, 2, 12, 3, 17, 10, 18, 6, 7, 19, 9, 13, 5, 4, 14, 11, 15, 8};
            unsigned int last = 0;
            std::vector<calib_sequence> c_sequences;
            uint8_t aux_counter = 0;
            int spc_calib = 0, bb_calib = 0;
            std::vector<uint16_t> PRT_counts[5];

        public:
            HIRSReader(int year);
            ~HIRSReader();
            int line = 0;
            void work(uint8_t *buffer);
            image::Image getChannel(int channel);
            void calibrate(nlohmann::json calib_coef, bool HIRS3);
            nlohmann::json calib_out;

            double last_timestamp = -1;
            TIPTimeParser ttp;
            std::vector<double> timestamps;
            int sync = 0;
        };
    } // namespace hirs
} // namespace noaa
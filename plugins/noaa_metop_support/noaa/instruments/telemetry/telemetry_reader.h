#pragma once

#include <vector>
#include <cstdint>
#include "../../tip_time_parser.h"
#include "nlohmann/json_utils.h"

namespace noaa
{
    namespace telemetry
    {
        class TelemetryReader
        {
        private:
            double lastTS = -1;
            TIPTimeParser ttp;
            nlohmann::json telemetry;

            // row buffers for subcoms
            std::array<int,320> analog1_buf;
            std::array<int,160> analog2_buf;
            std::array<int,10> analog3_buf;
            std::array<int,160> analog4_buf;
            std::array<int,256> digital1_buf;
            std::array<int,256> digital2_buf;
            std::array<int,10> dau1_buf;
            std::array<int,10> dau2_buf;
            std::array<int,16> satcu_buf;

            std::vector<double> timestamp_320;
            std::vector<double> timestamp_160;
            std::vector<double> timestamp_32;
            std::vector<double> timestamp_10;
            std::vector<double> timestamp_satcu;

            std::vector<std::array<int,320>> analog1;
            std::vector<std::array<int,160>> analog2;
            std::vector<std::array<int,10>> analog3;
            std::vector<std::array<int,160>> analog4;
            std::vector<std::array<int,256>> digital1;
            std::vector<std::array<int,256>> digital2;
            std::vector<std::array<int,10>> dau1;
            std::vector<std::array<int,10>> dau2;
            std::vector<std::array<int,16>> satcu;

        public:
            TelemetryReader(int year);
            ~TelemetryReader();
            void work(uint8_t *buffer);
            nlohmann::json dump_telemetry(bool is_n15);
        };
    }
}

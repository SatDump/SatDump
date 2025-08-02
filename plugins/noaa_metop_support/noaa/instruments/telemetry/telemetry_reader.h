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
            bool check_parity;

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
            TelemetryReader(int year, bool check_parity);
            ~TelemetryReader();
            void work(uint8_t *buffer);
            nlohmann::json dump_telemetry();

            // std::vector<std::array<int,16>> satcu;

            // std::vector<double> timestamp_avhrr;
            // std::vector<double> avhrr;
            // std::vector<double> timestamp_avhrr_temp;
            // std::vector<double> avhrr_temp;

        char* avhrr_telemetry_names[22] = {
                "Patch Temperature",
                "Patch Temperature Extended",
                "Patch Power",
                "Radiator Temperature",
                "Blackbody Temperature 1",
                "Blackbody Temperature 2",
                "Blackbody Temperature 3",
                "Blackbody Temperature 4",
                "Electronics Current",
                "Motor Current",
                "Earth Shield Position",
                "Electronics Temperature",
                "Cooler Housing Temperature",
                "Baseplate Temperature",
                "Motor Housing Temperature",
                "A/D Converter Temperature",
                "Detector #4 Bias Voltage",
                "Detector #5 Bias Voltage",
                "Blackbody Temperature Channel 3B",
                "Blackbody Temperature Channel 4",
                "Blackbody Temperature Channel 5",
                "Reference Voltage",
        };

            std::array<std::vector<double>,22> avhrr_timestamps;
            std::array<std::vector<double>,22> avhrr;

            unsigned int good_frames;
            unsigned int frames;
            unsigned int major_frames;

            void calibration(nlohmann::json coefficients);
        };
    }
}

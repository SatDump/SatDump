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
            int analog1_buf[320];
            int analog2_buf[160];
            int analog3_buf[10];
            int analog4_buf[160];
            int digital1_buf[256];
            int digital2_buf[256];
            int dau1_buf[10];
            int dau2_buf[10];

        public:
            TelemetryReader(int year);
            ~TelemetryReader();
            void work(uint8_t *buffer);
            nlohmann::json dump_telemetry();
        };
    }
}

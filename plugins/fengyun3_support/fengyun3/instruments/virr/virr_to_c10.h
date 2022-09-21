#pragma once

#include "common/ccsds/ccsds.h"
#include <fstream>

namespace fengyun3
{
    namespace virr
    {
        class VIRRToC10
        {
        private:
            std::string temp_path;
            std::ofstream output_c10;

            uint8_t c10_buffer[27728];

        public:
            VIRRToC10();
            ~VIRRToC10();
            void work(std::vector<uint8_t> &packet);
            void open(std::string path);
            void close(time_t timestamp, int satellite);
        };
    } // namespace avhrr
} // namespace metop
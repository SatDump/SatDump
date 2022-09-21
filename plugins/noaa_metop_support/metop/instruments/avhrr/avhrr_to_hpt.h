#pragma once

#include "common/ccsds/ccsds.h"
#include <fstream>

namespace metop
{
    namespace avhrr
    {
        class AVHRRToHpt
        {
        private:
            std::string temp_path;
            std::ofstream output_hpt;

            uint8_t hpt_buffer[13864];
            int counter = 0, counter2 = 0;

        public:
            AVHRRToHpt();
            ~AVHRRToHpt();
            void work(ccsds::CCSDSPacket &packet);
            void open(std::string path);
            void close(time_t timestamp, int satellite);
        };
    } // namespace avhrr
} // namespace metop
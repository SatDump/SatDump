#pragma once

#include "common/ccsds/ccsds.h"
#include <fstream>

namespace proba
{
    namespace gps_ascii
    {
        class GPSASCII
        {
        private:
            std::ofstream output_txt;

        public:
            GPSASCII(std::string path);
            ~GPSASCII();

            void work(ccsds::CCSDSPacket &packet);
        };
    } // namespace swap
} // namespace proba
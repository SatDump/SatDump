#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image/image.h"
#include "noaa/tip_time_parser.h"

namespace noaa_metop
{
    namespace avhrr
    {
        class AVHRRReader
        {
        private:
            uint16_t avhrr_buffer[10355];
            const bool gac_mode;
            const int width;
            noaa::TIPTimeParser ttp;
            void line2image(uint16_t *buff, int pos, int width, bool is_ch3a);

        public:
            int lines;
            std::vector<uint16_t> channels[6];
            std::vector<double> timestamps;

        public:
            AVHRRReader(bool gac);
            ~AVHRRReader();
            void work_metop(ccsds::CCSDSPacket &packet);
            void work_noaa(uint16_t *buffer);
            image::Image<uint16_t> getChannel(int channel);
        };
    } // namespace noaa_metop
} // namespace metop
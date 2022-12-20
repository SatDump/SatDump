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
            struct view_pair{
                uint16_t space;
                uint16_t blackbody;
            };
            uint16_t avhrr_buffer[10355];
            const bool gac_mode;
            const int width;
            noaa::TIPTimeParser ttp;
            void line2image(uint16_t *buff, int pos, int width, bool is_ch3a);
            std::vector<uint16_t> prt_buffer;
            std::vector<std::array<view_pair, 3>> views;

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
            int calibrate(std::string sat_name);
        };
    } // namespace noaa_metop
} // namespace metop
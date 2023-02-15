#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image/image.h"
#include "noaa/tip_time_parser.h"
#include "nlohmann/json.hpp"
#include "resources.h"
#include "common/calibration.h"

namespace noaa_metop
{
    namespace avhrr
    {
        class AVHRRReader
        {
        private:
            struct view_pair{
                uint16_t space = 0;
                uint16_t blackbody = 0;

                view_pair& operator+=(view_pair const &obj){
                    this->space += obj.space;
                    this->blackbody += obj.blackbody;

                    return *this;
                };

                view_pair& operator/=(int const n){
                    this->space /= n;
                    this->blackbody /= n;

                    return *this;
                };
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
            nlohmann::json calib_out;

        public:
            AVHRRReader(bool gac);
            ~AVHRRReader();
            void work_metop(ccsds::CCSDSPacket &packet);
            void work_noaa(uint16_t *buffer);
            image::Image<uint16_t> getChannel(int channel);
            void calibrate(nlohmann::json calib_coefs);
        };
    } // namespace noaa_metop
} // namespace metop
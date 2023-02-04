#pragma once

#include <cstdint>
#include <vector>

#include "common/image/image.h"
#include "common/ccsds/ccsds.h"
#include "common/simple_deframer.h"

namespace noaa_metop
{
    namespace amsu
    {
        class AMSUReader
        {
        public:
            std::vector<double> timestamps_A1;
            std::vector<double> timestamps_A2;
            double last_TIP_timestamp;
            int linesA1 = 0, linesA2 = 0;

        private:
            std::vector<uint16_t> channels[15];
            def::SimpleDeframer amsuA2Deframer = def::SimpleDeframer(0xFFFFFF, 24, 9920, 0);
            def::SimpleDeframer amsuA1Deframer = def::SimpleDeframer(0xFFFFFF, 24, 2496, 0);
            std::vector<std::vector<uint8_t>> amsuA2Data;
            std::vector<std::vector<uint8_t>> amsuA1Data;

        public:
            AMSUReader();
            ~AMSUReader();
            void work_noaa(uint8_t *buffer);
            void work_metop(ccsds::CCSDSPacket &packet);
            image::Image<uint16_t> getChannel(int channel)
            {
                image::Image<uint16_t> img(channels[channel].data(), 30, (channel < 2 ? linesA2 : linesA1), 1);
                img.mirror(true, false);
                return img;
            }

        private:
            void work_A1(uint8_t *buffer);
            void work_A2(uint8_t *buffer);

            bool contains(std::vector<double> tm, double n)
            {
                for (unsigned int i = 0; i < tm.size(); i++)
                {
                    if (tm[i] == n)
                        return true;
                }
                return false;
            }
        };
    }; // namespace amsu
};     // namespace noaa_metop
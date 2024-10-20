#pragma once

#include <cstdint>
#include "common/image/image.h"
#include "../../tip_time_parser.h"
#include "../../contains.h"

namespace noaa
{
    namespace hirs
    {
        class HIRSReader
        {
        private:
            std::vector<uint16_t> channels[20];
            // std::vector<uint16_t> imageBuffer[20];
            const int HIRSPositions[36] = {16, 17, 22, 23, 26, 27, 30, 31, 34, 35, 38, 39, 42, 43, 54, 55, 58, 59, 62, 63, 66, 67, 70, 71, 74, 75, 78, 79, 82, 83, 84, 85, 88, 89, 92, 93};
            const int HIRSChannels[20] = {0, 16, 1, 2, 12, 3, 17, 10, 18, 6, 7, 19, 9, 13, 5, 4, 14, 11, 15, 8};
            unsigned int last = 0;

        public:
            HIRSReader(int year);
            ~HIRSReader();
            int line = 0;
            void work(uint8_t *buffer);
            image::Image getChannel(int channel);

            double last_timestamp = -1;
            TIPTimeParser ttp;
            std::vector<double> timestamps;
            int sync = 0;
        };

        struct calib_sequence
        {
        public:
            uint16_t position = 0;
            uint16_t space = 0;
            uint16_t blackbody = 0;

        private:
            uint16_t calc_avg(uint16_t *samples, int count)
            {
                /*
                This function calcualtes the average value for space and bb looks, using the 3-sigma criterion to discard bad samples
                */
                double mean = 0;
                double variance = 0;

                // calculate the mean
                for (int i = 0; i < count; i++)
                    mean += samples[i];
                mean /= count;

                for (int i = 0; i < count; i++)
                    variance += pow(samples[i] - mean, 2) / count;

                std::pair<uint16_t, uint16_t> range = {mean - 3 * pow(variance, 0.5), mean + 3 * pow(variance, 0.5)};
                uint32_t avg = 0;
                uint8_t cnt = 0;

                for (int i = 0; i < count; i++)
                {
                    if (samples[i] >= range.first && samples[i] <= range.second)
                    {
                        avg += samples[i];
                        cnt++;
                    }
                }
                avg /= cnt;

                return avg;
            }
        };
    } // namespace hirs
} // namespace noaa
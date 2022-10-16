#pragma once

#include <vector>
#include <cstdint>
#include "../../tip_time_parser.h"

namespace noaa
{
    namespace sem
    {
        class SEMReader
        {
        private:
            std::vector<int> *channels[62];
            std::vector<double> *timestamps[62];
            double lastTS = -1;
            TIPTimeParser ttp;
            void pushChannelDataAndTS(int ch, uint8_t data, uint16_t frame)
            {
                channels[ch]->push_back(data ^ 0xFF);
                if (lastTS != -1)
                    timestamps[ch]->push_back(lastTS + ((double)frame / 10.0d));
                else
                    timestamps[ch]->push_back(-1);
            }

        public:
            const char *channel_names[62] =
                {
                    "0P1", "0P2", "0P3", "0P4", "0P5", "0P6", "0E1", "0E2",
                    "0E3", "9P1", "9P2", "9P3", "9P4", "9P5", "9P6", "9E1",
                    "9E2", "9E3", "P6", "P7", "P8", "P9", "0DE1", "0DE2",
                    "0DE3", "0DE4", "3DE1", "3DE2", "3DE3", "3DE4", "0DP1",
                    "0DP2", "0DP3", "0DP4", "3DP1", "3DP2", "3DP3", "3DP4",
                    "0EFL", "3EFL", "0PFL", "3PFL", "0EFH", "3EFH", "0PFH",
                    "3PFH", "0EM", "3EM", "0PM", "3PM", "0DEM", "3DEM",
                    "0DPM", "3DPM", "0PBKL", "0PBKH", "3PBKL", "3PBKH",
                    "0EBKL", "0EBKH", "3EBKL", "3EBKH"};

            SEMReader();
            ~SEMReader();
            void work(uint8_t *buffer);
            std::vector<int> getChannel(int channel);
            std::vector<double> getTimestamps(int channel);
            // std::vector<uint8_t> getChannelByName(std::string name);
        };
    }
}
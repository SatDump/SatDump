#pragma once

#include <map>
#include <vector>

namespace meteor
{
    namespace msumr
    {
        inline std::pair<double, double> get_x_offset(int msumr_id, int ch)
        {
            // MSU-MR offsets between channels for each instrument
            std::map<int, std::vector<std::pair<double, double>>> offsets = {
                {
                    0,
                    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
                },
                {
                    3, // Meteor-M N°2-3 channel 4 offset -1.6, -1.6
                    {{0, 0}, {0, 0}, {0, 0}, {-1.6, 0}, {0, 0}, {0, 0}},
                },
                {
                    4, // Meteor-M N°2-4 channel 4 offset -2, 1
                    {{0, 0}, {0, 0}, {0, 0}, {-2, 0}, {0, 0}, {0, 0}},
                },
            };

            if (offsets.count(msumr_id))
                return offsets[msumr_id][ch];
            return {0, 0};
        }
    } // namespace msumr
} // namespace meteor
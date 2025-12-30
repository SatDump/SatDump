#pragma once

#include <map>
#include <vector>

namespace meteor
{
    namespace msumr
    {
        inline int get_x_offset(int msumr_id, int ch)
        {
            // MSU-MR offsets between channels for each instrument
            std::map<int, std::vector<double>> offsets = {
                {
                    0,
                    {0, 0, 0, 0, 0, 0},
                },
            };

            if (offsets.count(msumr_id))
                return offsets[msumr_id][ch];
            return 0;
        }
    } // namespace msumr
} // namespace meteor
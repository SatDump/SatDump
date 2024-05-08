#include "histogram_utils.h"
#include <limits>

#include "logger.h"

namespace image2
{
    namespace histogram
    {
        std::vector<int> get_histogram(std::vector<int> values, int len)
        {
            std::vector<int> final_hist(len, 0);

            // Compute histogram
            for (int px = 0; px < (int)values.size(); px++)
                final_hist[values[px]] += 1;

            return final_hist;
        }

        std::vector<int> equalize_histogram(std::vector<int> hist)
        {
            std::vector<int> equ_hist(hist.size());
            equ_hist[0] = hist[0];
            for (int i = 1; i < (int)hist.size(); i++)
                equ_hist[i] = hist[i] + equ_hist[i - 1];
            return equ_hist;
        }

        int try_find_val(std::vector<int> &array, int val)
        {
            int pos = -1;
            for (int i = 0; i < (int)array.size(); i++)
            {
                if (array[i] == val)
                {
                    pos = i;
                    return pos;
                }
            }
            return pos;
        }

        /*int try_find_val_around(std::vector<int> &array, int cent_pos, int val)
        {
            int array_size = array.size();
            for (int i = 0; i < array_size; i++)
            {
                int pos_min = cent_pos - i;
                int pos_max = cent_pos + i;

                // if (pos_min >= 0 && pos_min < array_size)
                //     if (array[pos_min] == val)
                //         return pos_min;

                if (pos_max >= 0 && pos_max < array_size)
                    if (array[pos_max] == val)
                        return pos_max;
            }
            return try_find_val(array, val);
        }*/

        int find_target_value_hist(std::vector<int> &array, int cent_pos, int val)
        {
            if (val == std::numeric_limits<int>::max())
                return -1;
            if (val < 0)
                return -1;

            int pos = try_find_val(array, val);
            if (pos == -1)
            {
                pos = find_target_value_hist(array, cent_pos, val + 1);
                if (pos == -1)
                    pos = find_target_value_hist(array, cent_pos, val - 1);
            }
            return pos;
        }

        std::vector<int> make_hist_match_table(std::vector<int> input_hist, std::vector<int> target_hist, int /*maxdiff*/)
        {
            std::vector<int> table(target_hist.size());
            for (int i = 0; i < (int)target_hist.size(); i++)
            {
                table[i] = find_target_value_hist(target_hist, i, input_hist[i]);
                if (i != 0 && table[i] == 0)
                    table[i] = i;
            }
            table[0] = 0; // To be sure
            return table;
        }
    }
}
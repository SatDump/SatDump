#include "histogram_utils.h"
#include <limits>

namespace image
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
            for (int i = 0; i < array.size(); i++)
            {
                if (array[i] == val)
                {
                    pos = i;
                    return pos;
                }
            }
            return pos;
        }

        int find_target_value_hist(std::vector<int> &array, int val)
        {
            if (val == std::numeric_limits<int>::max())
                return -1;
            if (val < 0)
                return -1;

            int pos = try_find_val(array, val);
            if (pos == -1)
            {
                pos = find_target_value_hist(array, val + 1);
                if (pos == -1)
                    pos = find_target_value_hist(array, val - 1);
            }
            return pos;
        }

        std::vector<int> make_hist_match_table(std::vector<int> input_hist, std::vector<int> target_hist, int maxdiff)
        {
            std::vector<int> table(target_hist.size());
            for (int i = 0; i < (int)target_hist.size(); i++)
            {
                table[i] = find_target_value_hist(target_hist, input_hist[i]);
                if (i != 0 && (table[i] == 0 || table[i] == -1)) // Eg, if it starts late due to some unused values!
                    table[i] = i;
            }

            table[0] = 0; // To be sure

            // Check continuity for "gaps"
            for (int i = 1; i < (int)target_hist.size() - 1; i++)
            {
                int previous = table[i - 1];
                int current = table[i];
                int next = table[i + 1];

                int diff1 = abs(previous - current);
                int diff2 = abs(next - current);

                if (diff1 > maxdiff && diff2 > maxdiff)
                    table[i] = (previous + next) / 2;
            }

            return table;
        }
    }
}
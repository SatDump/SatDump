#pragma once

#include "image.h"
#include <vector>

namespace image
{
    namespace histogram
    {
        // Get histogram of a buffer
        std::vector<int> get_histogram(std::vector<int> values, int len);

        // Equalize an histogram (stretch to normal distribution)
        std::vector<int> equalize_histogram(std::vector<int> hist);

        // Make an histogram-matching table between 2 equalized histograms
        std::vector<int> make_hist_match_table(std::vector<int> input_hist, std::vector<int> target_hist, int maxdiff);
    }
}
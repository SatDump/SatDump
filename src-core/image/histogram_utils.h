#pragma once

/**
 * @file histogram_utils.h
 * @brief Somewhat generic histogram manipulation utilities
 */

#include "image.h"
#include <vector>

namespace satdump
{
    namespace image
    {
        namespace histogram
        {
            /**
             * @brief Get histogram of a buffer
             * @param values to get the history of
             * @param len length of the output histogram
             * @return histogram
             */
            std::vector<int> get_histogram(std::vector<int> values, int len);

            /**
             * @brief Equalize an histogram (stretch to normal distribution)
             * @param hist histogram to equalize
             * @return equalized histogram
             */
            std::vector<int> equalize_histogram(std::vector<int> hist);

            /**
             * @brief Make an histogram-matching table between 2 equalized histograms
             * @param input_hist histogram to match from
             * @param target_hist histogram to match to
             * @param maxdiff Ignored (TODOREWORK?)
             * @return matching table LUT
             */
            std::vector<int> make_hist_match_table(std::vector<int> input_hist, std::vector<int> target_hist, int maxdiff);
        } // namespace histogram
    } // namespace image
} // namespace satdump
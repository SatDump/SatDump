#pragma once

/**
 * @file stats.h
 * @brief Math/Statistics functions
 */

#include <algorithm>
#include <map>
#include <vector>

namespace satdump
{
    /**
     * @brief Return the most common element within the
     * provided sequence.
     *
     * @param begin First iterator
     * @param end last iterator
     * @param def default if we don't have enough elements
     * @return most common element OR default
     */
    template <class InputIt, class T = typename std::iterator_traits<InputIt>::value_type>
    T most_common(InputIt begin, InputIt end, T def)
    {
        if (begin == end)
            return def;

        std::map<T, int> counts;
        for (InputIt it = begin; it != end; ++it)
        {
            if (counts.find(*it) != counts.end())
                ++counts[*it];
            else
                counts[*it] = 1;
        }
        return std::max_element(counts.begin(), counts.end(), [](const std::pair<T, int> &pair1, const std::pair<T, int> &pair2) { return pair1.second < pair2.second; })->first;
    }

    /**
     * @brief Returns the average of the provided
     * elements.
     *
     * @param begin First iterator
     * @param end last iterator
     * @return average of elements
     */
    template <class InputIt, class T = typename std::iterator_traits<InputIt>::value_type>
    T average_common(InputIt begin, InputIt end)
    {
        T avg = 0;
        size_t count = 0;
        for (InputIt it = begin; it != end; ++it)
        {
            avg += *it;
            count++;
        }
        return avg / T(count);
    }

    /**
     * @brief Returns a percentile of the provided array
     *
     * @param array to work with
     * @param size of the array, in elements
     * @param percentile to return
     * @return the percentile
     */
    template <typename T>
    T percentile(T *array, int size, float percentile)
    {
        float number_percent = (size + 1) * percentile / 100.0f;
        if (number_percent == 1)
            return array[0];
        else if (number_percent == size)
            return array[size - 1];
        else
            return array[(int)number_percent - 1] + (number_percent - (int)number_percent) * (array[(int)number_percent] - array[(int)number_percent - 1]);
    }

    /**
     * @brief Returns the average of the provided
     * elements. Version that won't overflow with
     * very large numbers and amount of elements.
     *
     * @param v Vector to average
     * @return average of elements
     */
    template <typename T>
    double avg_overflowless(std::vector<T> const &v)
    {
        T n = 0;
        double mean = 0.0;
        for (auto x : v)
        {
            double delta = x - mean;
            mean += delta / ++n;
        }
        return mean;
    }

    /**
     * @brief Returns the median of the provided
     * elements.
     *
     * @param values input vector
     * @return median of the elements
     */
    double get_median(std::vector<double> values);
} // namespace satdump
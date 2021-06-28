#pragma once

#include <cstdint>
#include <algorithm>
#include <map>
#include <string>
#include <vector>

void char_array_to_uchar(int8_t *in, uint8_t *out, int nsamples);

template <class InputIt, class T = typename std::iterator_traits<InputIt>::value_type>
T most_common(InputIt begin, InputIt end)
{
    std::map<T, int> counts;
    for (InputIt it = begin; it != end; ++it)
    {
        if (counts.find(*it) != counts.end())
            ++counts[*it];
        else
            counts[*it] = 1;
    }
    return std::max_element(counts.begin(), counts.end(), [](const std::pair<T, int> &pair1, const std::pair<T, int> &pair2)
                            { return pair1.second < pair2.second; })
        ->first;
}

std::vector<std::string> splitString(std::string input, char del);
#pragma once

#include <cstdint>
#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <climits>
#include <fstream>
#include <optional>

void char_array_to_uchar(int8_t *in, uint8_t *out, int nsamples);
void signed_soft_to_unsigned(int8_t *in, uint8_t *out, int nsamples);

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

template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 6)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return out.str();
}

template <typename T>
T swap_endian(T u)
{
    union
    {
        T u;
        unsigned char u8[sizeof(T)];
    } source, dest;

    source.u = u;

    for (size_t k = 0; k < sizeof(T); k++)
        dest.u8[k] = source.u8[sizeof(T) - k - 1];

    return dest.u;
}

template <typename T>
int percentile(T *array, int size, float percentile)
{
    float number_percent = (size + 1) * percentile / 100.0f;
    if (number_percent == 1)
        return array[0];
    else if (number_percent == size)
        return array[size - 1];
    else
        return array[(int)number_percent - 1] + (number_percent - (int)number_percent) * (array[(int)number_percent] - array[(int)number_percent - 1]);
}

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

template <typename T>
double avg_overflowless_timestamps(std::vector<T> const &v)
{
    T n = 0;
    double mean = 0.0;
    for (auto x : v)
    {
        if (x != -1)
        {
            double delta = x - mean;
            mean += delta / ++n;
        }
    }
    return mean;
}

std::vector<std::string> splitString(std::string input, char del);

template <typename T>
inline bool getBit(T &data, int &bit)
{
    return (data >> bit) & 1;
}

bool isStringPresent(std::string searched, std::string keyword);

// Return filesize
size_t getFilesize(std::string filepath);

// Perform a HTTP Request on the provided URL and return the result as a string
int perform_http_request(std::string url, std::string &result);

std::string timestamp_to_string(double timestamp);

inline std::vector<float> double_buffer_to_float(double *ptr, int size)
{
    std::vector<float> ret;
    for (int i = 0; i < size; i++)
        ret.push_back(ptr[i]);
    return ret;
}

double get_median(std::vector<double> values);

extern "C" time_t mktime_utc(const struct tm *timeinfo_utc); // Already in libpredict!

template <typename T>
std::vector<uint8_t> unsigned_to_bitvec(T v)
{
    std::vector<uint8_t> c;
    for (int s = (sizeof(T) * 8) - 1; s >= 0; s--)
        c.push_back((v >> s) & 1);
    return c;
}

std::string loadFileToString(std::string path);
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
#include <chrono>

void char_array_to_uchar(int8_t *in, uint8_t *out, int nsamples);
void signed_soft_to_unsigned(int8_t *in, uint8_t *out, int nsamples);

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
uint64_t getFilesize(std::string filepath);

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

template <typename T>
std::vector<T> oversample_vector(std::vector<T> data, int oversampling)
{
    std::vector<T> r;
    for (T &v : data)
        for (int i = 0; i < oversampling; i++)
            r.push_back(v);
    return r;
}

template <typename... T>
std::string svformat(const char *fmt, T &&...args)
{
    // Allocate a buffer on the stack that's big enough for us almost
    // all the time.
    size_t size = 1024;
    std::vector<char> buf;
    buf.resize(size);

    // Try to vsnprintf into our buffer.
    size_t needed = snprintf((char *)&buf[0], size, fmt, args...);
    // NB. On Windows, vsnprintf returns -1 if the string didn't fit the
    // buffer.  On Linux & OSX, it returns the length it would have needed.

    if (needed <= size)
    {
        // It fit fine the first time, we're done.
        return std::string(&buf[0]);
    }
    else
    {
        // vsnprintf reported that it wanted to write more characters
        // than we allotted.  So do a malloc of the right size and try again.
        // This doesn't happen very often if we chose our initial size
        // well.
        size = needed;
        buf.resize(size);
        needed = snprintf((char *)&buf[0], size, fmt, args...);
        return std::string(&buf[0]);
    }
}

inline double getTime()
{
    auto time = std::chrono::system_clock::now();
    auto since_epoch = time.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch);
    return millis.count() / 1e3;
}

std::string loadFileToString(std::string path);
std::string ws2s(const std::wstring &wstr);
std::wstring s2ws(const std::string &str);

std::string prepareAutomatedPipelineFolder(time_t timevalue, double frequency, std::string pipeline_name, std::string folder = "");
std::string prepareBasebandFileName(double timeValue_precise, uint64_t samplerate, uint64_t frequency);

void hsv_to_rgb(float h, float s, float v, uint8_t *rgb);
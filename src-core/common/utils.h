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

#ifdef _MSC_VER
#define timegm _mkgmtime
#endif

void char_array_to_uchar(int8_t *in, uint8_t *out, int nsamples);
void signed_soft_to_unsigned(int8_t *in, uint8_t *out, int nsamples);





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
inline bool getBit(T &data, int &bit)
{
    return (data >> bit) & 1;
}

// Return filesize
uint64_t getFilesize(std::string filepath);

inline std::vector<float> double_buffer_to_float(double *ptr, int size)
{
    std::vector<float> ret;
    for (int i = 0; i < size; i++)
        ret.push_back(ptr[i]);
    return ret;
}



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








std::string prepareAutomatedPipelineFolder(time_t timevalue, double frequency, std::string pipeline_name, std::string folder = "");
std::string prepareBasebandFileName(double timeValue_precise, uint64_t samplerate, uint64_t frequency);

void hsv_to_rgb(float h, float s, float v, uint8_t *rgb);

inline int ensureIs2Multiple(int v)
{
    if (v % 2 == 1)
        v++;
    return v;
}
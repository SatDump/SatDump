/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "logger.h"

#include <fstream>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>

#include "common/ccsds/ccsds_weather/vcdu.h"
#include "common/ccsds/ccsds_weather/demuxer.h"

#include "common/image/jpeg12_utils.h"

#include "common/codings/reedsolomon/reedsolomon.h"

#if 0
template <typename T>
std::vector<size_t> idl_where(std::vector<T> x, T y)
{
    std::vector<size_t> v;
    for (size_t i = 0; i < x.size(); i++)
        if (x[i] == y)
            v.push_back(i);
    return v;
}

void image_decomp(image::Image<uint16_t> &img)
{
    float a0 = 1877.5;
    float b0 = 341.0;
    float c0 = -669298;
    float Nc0 = 2048;

    std::vector<float> data(img.width() * img.height());

    for (size_t i = 0; i < img.width() * img.height(); i++)
        data[i] = img[i]; //>> 4;

    std::vector<float> output = data; //(img.width() * img.height());

    auto ss = idl_where<float>(data, Nc0);
    if (ss.size() > 0)
    {
        for (auto &ssi : ss)
            output[ssi] = round(((data[ssi] - a0) * (data[ssi] - a0) - c0) / b0);
    }

    auto sss = idl_where<float>(data, 16383);
    if (sss.size() > 0)
    {
        for (auto &sssi : sss)
            output[sssi] = 16383;
    }

    for (size_t i = 0; i < img.width() * img.height(); i++)
        img[i] = int(output[i]); //<< 4;
}
#endif

int main(int /*argc*/, char * /*argv*/[])
{
    initLogger();
}
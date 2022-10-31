#pragma once

#include <cstdint>
#include <vector>
#include <functional>

namespace dsp
{
    namespace window
    {
        double cosine(double n, double N, const double *coefs, int coefCount);
        double nuttall(double n, double N);
    }

    double sinc(double x);
    std::vector<float> windowed_sinc(int count, double omega, std::function<double(double, double)> window, double norm = 1.0);
} // namespace dsp

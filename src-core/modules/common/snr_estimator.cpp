#include "snr_estimator.h"

M2M4SNREstimator::M2M4SNREstimator(float alpha)
{
    d_y1 = 0;
    d_y2 = 0;

    d_signal = 0;
    d_noise = 0;

    d_alpha = alpha;
    d_beta = 1.0 - alpha;
}

void M2M4SNREstimator::update(std::complex<float> *input, int size)
{
    for (int i = 0; i < size; i++)
    {
        float y1 = abs(input[i]) * abs(input[i]);
        d_y1 = d_alpha * y1 + d_beta * d_y1;

        float y2 = abs(input[i]) * abs(input[i]) * abs(input[i]) * abs(input[i]);
        d_y2 = d_alpha * y2 + d_beta * d_y2;
    }
}

float M2M4SNREstimator::snr()
{
    float y1_2 = d_y1 * d_y1;
    d_signal = sqrt(2 * y1_2 - d_y2);
    d_noise = d_y1 - sqrt(2 * y1_2 - d_y2);
    return std::max<float>(0, 10.0 * log10(d_signal / d_noise));
}

float M2M4SNREstimator::signal()
{
    return 10.0 * log10(d_signal);
}

float M2M4SNREstimator::noise()
{
    return 10.0 * log10(d_noise);
}
#include "mmse_fir_interpolator.h"
#include "interpolator_taps.h"

namespace dsp
{
    std::vector<fir_filter_ccf> build_filters_cc()
    {
        std::vector<fir_filter_ccf> filters;
        filters.reserve(NSTEPS + 1);
        for (int i = 0; i < NSTEPS + 1; i++)
        {
            std::vector<float> t(&taps[i][0], &taps[i][NTAPS]);
            filters.emplace_back(1, t);
        }
        return filters;
    }

    std::vector<fir_filter_fff> build_filters_ff()
    {
        std::vector<fir_filter_fff> filters;
        filters.reserve(NSTEPS + 1);
        for (int i = 0; i < NSTEPS + 1; i++)
        {
            std::vector<float> t(&taps[i][0], &taps[i][NTAPS]);
            filters.emplace_back(1, t);
        }
        return filters;
    }

    mmse_fir_interpolator_cc::mmse_fir_interpolator_cc() : filters(build_filters_cc()) {}

    mmse_fir_interpolator_cc::~mmse_fir_interpolator_cc() {}

    unsigned mmse_fir_interpolator_cc::ntaps() const { return NTAPS; }

    unsigned mmse_fir_interpolator_cc::nsteps() const { return NSTEPS; }

    std::complex<float> mmse_fir_interpolator_cc::interpolate(const std::complex<float> input[], float mu) const
    {
        int imu = (int)rint(mu * NSTEPS);

        if ((imu < 0) || (imu > NSTEPS))
        {
            throw std::runtime_error("mmse_fir_interpolator_ccf: imu out of bounds. " + std::to_string(mu));
        }

        std::complex<float> r = filters[imu].filter(input);
        return r;
    }

    mmse_fir_interpolator_ff::mmse_fir_interpolator_ff() : filters(build_filters_ff()) {}

    mmse_fir_interpolator_ff::~mmse_fir_interpolator_ff() {}

    unsigned mmse_fir_interpolator_ff::ntaps() const { return NTAPS; }

    unsigned mmse_fir_interpolator_ff::nsteps() const { return NSTEPS; }

    float mmse_fir_interpolator_ff::interpolate(const float input[], float mu) const
    {
        int imu = (int)rint(mu * NSTEPS);

        if ((imu < 0) || (imu > NSTEPS))
        {
            throw std::runtime_error("mmse_fir_interpolator_fff: imu out of bounds. " + std::to_string(mu));
        }

        float r = filters[imu].filter(input);
        return r;
    }
} // namespace libdsp
#pragma once

#include "fir_filter_imp.h"

namespace dsp
{
    class mmse_fir_interpolator_cc
    {
    public:
        mmse_fir_interpolator_cc();
        ~mmse_fir_interpolator_cc();

        unsigned ntaps() const;
        unsigned nsteps() const;

        /*!
     * \brief compute a single interpolated output value.
     *
     * \p input must have ntaps() valid entries and be 8-byte aligned.
     * input[0] .. input[ntaps() - 1] are referenced to compute the output value.
     * \throws std::invalid_argument if input is not 8-byte aligned.
     *
     * \p mu must be in the range [0, 1] and specifies the fractional delay.
     *
     * \returns the interpolated input value.
     */
        std::complex<float> interpolate(const std::complex<float> input[], float mu) const;

    protected:
        const std::vector<fir_filter_ccf> filters;
    };

    class mmse_fir_interpolator_ff
    {
    public:
        mmse_fir_interpolator_ff();
        ~mmse_fir_interpolator_ff();

        unsigned ntaps() const;
        unsigned nsteps() const;

        /*!
     * \brief compute a single interpolated output value.
     *
     * \p input must have ntaps() valid entries and be 8-byte aligned.
     * input[0] .. input[ntaps() - 1] are referenced to compute the output value.
     * \throws std::invalid_argument if input is not 8-byte aligned.
     *
     * \p mu must be in the range [0, 1] and specifies the fractional delay.
     *
     * \returns the interpolated input value.
     */
        float interpolate(const float input[], float mu) const;

    protected:
        const std::vector<fir_filter_fff> filters;
    };
} // namespace libdsp
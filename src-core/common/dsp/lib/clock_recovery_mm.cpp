#include "clock_recovery_mm.h"
#include "utils.h"

static const int FUDGE = 16;

namespace dsp
{
    ClockRecoveryMMCC::ClockRecoveryMMCC(float samplerate, float symbolrate) : d_last_sample(0),
                                                                               d_p_2T(0),
                                                                               d_p_1T(0),
                                                                               d_p_0T(0),
                                                                               d_c_2T(0),
                                                                               d_c_1T(0),
                                                                               d_c_0T(0)
    {
        d_omega = samplerate / symbolrate;
        d_mu = 0.5f;
        d_omega_relative_limit = 0.005f;
        float damp = sqrtf(2.0f) / 2.0f;
        d_gain_mu = (4 * damp * d_omega_relative_limit) / (1.0 + 2.0 * damp * d_omega_relative_limit + d_omega_relative_limit * d_omega_relative_limit);
        d_gain_omega = (4 * d_omega_relative_limit * d_omega_relative_limit) / (1.0 + 2.0 * damp * d_omega_relative_limit + d_omega_relative_limit * d_omega_relative_limit);

        if (d_omega <= 0.0)
            throw std::out_of_range("clock rate must be > 0");
        if (d_gain_mu < 0 || d_gain_mu < 0)
            throw std::out_of_range("Gains must be non-negative");

        set_omega(d_omega); // also sets min and max omega
    }

    ClockRecoveryMMCC::ClockRecoveryMMCC(float omega, float gain_omega, float mu, float gain_mu, float omega_relative_limit)
        : d_mu(mu),
          d_omega(omega),
          d_gain_omega(gain_omega),
          d_omega_relative_limit(omega_relative_limit),
          d_gain_mu(gain_mu),
          d_last_sample(0),
          d_p_2T(0),
          d_p_1T(0),
          d_p_0T(0),
          d_c_2T(0),
          d_c_1T(0),
          d_c_0T(0)
    {
        if (omega <= 0.0)
            throw std::out_of_range("clock rate must be > 0");
        if (gain_mu < 0 || gain_omega < 0)
            throw std::out_of_range("Gains must be non-negative");

        set_omega(omega); // also sets min and max omega
    }

    ClockRecoveryMMCC::~ClockRecoveryMMCC()
    {
    }

    void ClockRecoveryMMCC::set_omega(float omega)
    {
        d_omega = omega;
        d_omega_mid = omega;
        d_omega_lim = d_omega_relative_limit * omega;
    }

    int ClockRecoveryMMCC::work(std::complex<float> *in, int length, std::complex<float> *out)
    {
        tmp_in.insert(tmp_in.end(), in, &in[length]);

        int ii = 0;                                        // input index
        int oo = 0;                                        // output index
        int ni = tmp_in.size() - d_interp.ntaps() - FUDGE; // don't use more input than this

        float mm_val = 0;
        std::complex<float> u, x, y;

        while (ii < ni)
        {
            d_p_2T = d_p_1T;
            d_p_1T = d_p_0T;
            d_p_0T = d_interp.interpolate(&tmp_in[ii], d_mu);

            d_c_2T = d_c_1T;
            d_c_1T = d_c_0T;
            d_c_0T = slicer_0deg(d_p_0T);

            fast_cc_multiply(x, d_c_0T - d_c_2T, conj(d_p_1T));
            fast_cc_multiply(y, d_p_0T - d_p_2T, conj(d_c_1T));
            u = y - x;
            mm_val = u.real();
            out[oo++] = d_p_0T;

            // limit mm_val
            mm_val = branchless_clip(mm_val, 1.0);

            d_omega = d_omega + d_gain_omega * mm_val;
            d_omega = d_omega_mid + branchless_clip(d_omega - d_omega_mid, d_omega_lim);

            d_mu = d_mu + d_omega + d_gain_mu * mm_val;
            ii += (int)floor(d_mu);
            d_mu -= floor(d_mu);

            if (ii < 0) // clamp it.  This should only happen with bogus input
                ii = 0;
        }

        if (ii <= (int)tmp_in.size())
            tmp_in.erase(tmp_in.begin(), tmp_in.begin() + ii);
        else
            throw std::out_of_range("ii out of bounds! Something is probably very wrong.");

        return oo;
    }

    ClockRecoveryMMFF::ClockRecoveryMMFF(float omega, float gain_omega, float mu, float gain_mu, float omega_relative_limit)
        : d_mu(mu),
          d_omega(omega),
          d_gain_omega(gain_omega),
          d_omega_relative_limit(omega_relative_limit),
          d_gain_mu(gain_mu),
          d_last_sample(0)
    {
        if (omega < 1)
            throw std::out_of_range("clock rate must be > 0");
        if (gain_mu < 0 || gain_omega < 0)
            throw std::out_of_range("Gains must be non-negative");

        set_omega(omega); // also sets min and max omega
    }

    ClockRecoveryMMFF::~ClockRecoveryMMFF()
    {
    }

    void ClockRecoveryMMFF::set_omega(float omega)
    {
        d_omega = omega;
        d_omega_mid = omega;
        d_omega_lim = d_omega_relative_limit * omega;
    }

    int ClockRecoveryMMFF::work(float *in, int length, float *out)
    {
        tmp_in.insert(tmp_in.end(), in, &in[length]);

        int ii = 0;                                // input index
        int oo = 0;                                // output index
        int ni = tmp_in.size() - d_interp.ntaps(); // don't use more input than this
        float mm_val;

        while (ii < ni)
        {
            // produce output sample
            out[oo] = d_interp.interpolate(&tmp_in[ii], d_mu);
            mm_val = slice(d_last_sample) * out[oo] - slice(out[oo]) * d_last_sample;
            d_last_sample = out[oo];

            d_omega = d_omega + d_gain_omega * mm_val;
            d_omega = d_omega_mid + branchless_clip(d_omega - d_omega_mid, d_omega_lim);
            d_mu = d_mu + d_omega + d_gain_mu * mm_val;

            ii += (int)floor(d_mu);
            d_mu = d_mu - floor(d_mu);
            oo++;
        }

        tmp_in.erase(tmp_in.begin(), tmp_in.begin() + ii);

        return oo;
    }
} // namespace libdsp
#include "pll_carrier_tracking.h"
#include "utils.h"

#define M_TWOPI (2 * M_PI)

namespace dsp
{
    PLLCarrierTracking::PLLCarrierTracking(float loop_bw,
                                           float max_freq,
                                           float min_freq)
        : d_locksig(0),
          d_lock_threshold(0),
          d_squelch_enable(false),
          ControlLoop(loop_bw, max_freq, min_freq)
    {
    }

    PLLCarrierTracking::~PLLCarrierTracking() {}

    bool PLLCarrierTracking::squelch_enable(bool set_squelch)
    {
        return d_squelch_enable = set_squelch;
    }

    float PLLCarrierTracking::set_lock_threshold(float threshold)
    {
        return d_lock_threshold = threshold;
    }

    size_t PLLCarrierTracking::work(std::complex<float> *in, size_t length, std::complex<float> *out)
    {
        float error;
        float t_imag, t_real;

        for (int i = 0; i < length; i++)
        {
            sincosf(d_phase, &t_imag, &t_real);

            fast_cc_multiply(out[i], in[i], std::complex<float>(t_real, -t_imag));

            error = phase_detector(in[i], d_phase);

            advance_loop(error);
            phase_wrap();
            frequency_limit();

            d_locksig = d_locksig * (1.0 - d_alpha) +
                        d_alpha * (in[i].real() * t_real + in[i].imag() * t_imag);

            if ((d_squelch_enable) && !lock_detector())
                out[i] = 0;
        }

        return length;
    }

    void PLLCarrierTracking::set_loop_bandwidth(float bw)
    {
        ControlLoop::set_loop_bandwidth(bw);
    }

    void PLLCarrierTracking::set_damping_factor(float df)
    {
        ControlLoop::set_damping_factor(df);
    }

    void PLLCarrierTracking::set_alpha(float alpha)
    {
        ControlLoop::set_alpha(alpha);
    }

    void PLLCarrierTracking::set_beta(float beta)
    {
        ControlLoop::set_beta(beta);
    }

    void PLLCarrierTracking::set_frequency(float freq)
    {
        ControlLoop::set_frequency(freq);
    }

    void PLLCarrierTracking::set_phase(float phase)
    {
        ControlLoop::set_phase(phase);
    }

    void PLLCarrierTracking::set_min_freq(float freq)
    {
        ControlLoop::set_min_freq(freq);
    }

    void PLLCarrierTracking::set_max_freq(float freq)
    {
        ControlLoop::set_max_freq(freq);
    }

    float PLLCarrierTracking::get_loop_bandwidth() const
    {
        return ControlLoop::get_loop_bandwidth();
    }

    float PLLCarrierTracking::get_damping_factor() const
    {
        return ControlLoop::get_damping_factor();
    }

    float PLLCarrierTracking::get_alpha() const
    {
        return ControlLoop::get_alpha();
    }

    float PLLCarrierTracking::get_beta() const
    {
        return ControlLoop::get_beta();
    }

    float PLLCarrierTracking::get_frequency() const
    {
        return ControlLoop::get_frequency();
    }

    float PLLCarrierTracking::get_phase() const
    {
        return ControlLoop::get_phase();
    }

    float PLLCarrierTracking::get_min_freq() const
    {
        return ControlLoop::get_min_freq();
    }

    float PLLCarrierTracking::get_max_freq() const
    {
        return ControlLoop::get_max_freq();
    }
} // namespace libdsp
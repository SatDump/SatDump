#pragma once
#include <complex>
#include <vector>
#include "control_loop.h"
#include "fast_atan2f.h"

/* Based on the Costas Loop from GNU Radio (https://github.com/gnuradio/gnuradio) */

namespace dsp
{
    class PLLCarrierTracking : public ControlLoop
    {
    private:
        float d_locksig, d_lock_threshold;
        bool d_squelch_enable;

        float mod_2pi(float in)
        {
            if (in > M_PI)
                return in - (2.0 * M_PI);
            else if (in < -M_PI)
                return in + (2.0 * M_PI);
            else
                return in;
        }

        float phase_detector(std::complex<float> sample, float ref_phase)
        {
            float sample_phase;
            //  sample_phase = atan2(sample.imag(),sample.real());
            sample_phase = fast_atan2f(sample.imag(), sample.real());
            return mod_2pi(sample_phase - ref_phase);
        }

    public:
        PLLCarrierTracking(float loop_bw, float max_freq, float min_freq);
        ~PLLCarrierTracking();

        bool lock_detector(void) { return (fabsf(d_locksig) > d_lock_threshold); }

        bool squelch_enable(bool);
        float set_lock_threshold(float);

        void set_loop_bandwidth(float bw);
        void set_damping_factor(float df);
        void set_alpha(float alpha);
        void set_beta(float beta);
        void set_frequency(float freq);
        void set_phase(float phase);
        void set_min_freq(float freq);
        void set_max_freq(float freq);

        float get_loop_bandwidth();
        float get_damping_factor();
        float get_alpha();
        float get_beta();
        float get_frequency();
        float get_phase();
        float get_min_freq();
        float get_max_freq();

        int work(std::complex<float> *in, int length, std::complex<float> *out);
    };
} // namespace libdsp
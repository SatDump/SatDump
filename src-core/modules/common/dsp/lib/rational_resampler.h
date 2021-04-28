#pragma once

#include <complex>
#include "fir_filter.h"

namespace dsp
{
    class RationalResamplerCCF
    {
    private:
        unsigned d_history;
        unsigned d_decimation;
        unsigned d_ctr;
        std::vector<float> d_new_taps;
        std::vector<fir_filter_ccf> d_firs;
        bool d_updated;
        volk::vector<std::complex<float>> tmp_;

        void install_taps(const std::vector<float> &taps);

    public:
        RationalResamplerCCF(unsigned interpolation, unsigned decimation);
        RationalResamplerCCF(unsigned interpolation, unsigned decimation, const std::vector<float> &taps);

        unsigned history() const { return d_history; }
        void set_history(unsigned history) { d_history = history; }

        unsigned interpolation() const { return d_firs.size(); }
        unsigned decimation() const { return d_decimation; }

        void set_taps(const std::vector<float> &taps);
        std::vector<float> taps() const;

        int work(std::complex<float> *in, int length, std::complex<float> *out);
    };
}; // namespace dsp
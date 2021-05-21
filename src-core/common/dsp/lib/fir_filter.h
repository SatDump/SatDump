#pragma once

#include "fir_filter_imp.h"

namespace dsp
{
    class FIRFilterCCF
    {
    private:
        fir_filter_ccf *d_fir;
        bool d_updated;
        int decimation_m;
        volk::vector<std::complex<float>> tmp_;

    public:
        FIRFilterCCF(int decimation, const std::vector<float> &taps);

        ~FIRFilterCCF();

        void set_taps(const std::vector<float> &taps);
        std::vector<float> taps() const;

        int work(std::complex<float> *in, int length, std::complex<float> *out);
    };

    class FIRFilterFFF
    {
    private:
        fir_filter_fff *d_fir;
        bool d_updated;
        int decimation_m;
        volk::vector<float> tmp_;

    public:
        FIRFilterFFF(int decimation, const std::vector<float> &taps);

        ~FIRFilterFFF();

        void set_taps(const std::vector<float> &taps);
        std::vector<float> taps() const;

        int work(float *in, int length, float *out);
    };
} // namespace libdsp
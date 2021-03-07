#pragma once

#include <complex>
#include <vector>
#include <volk/volk_alloc.hh>

namespace dsp
{
    class fir_filter_ccf
    {
    public:
        fir_filter_ccf(int decimation, const std::vector<float> &taps);
        ~fir_filter_ccf();

        void set_taps(const std::vector<float> &taps);
        void update_tap(float t, unsigned int index);
        std::vector<float> taps() const;
        unsigned int ntaps() const;

        std::complex<float> filter(const std::complex<float> input[]) const;
        void filterN(std::complex<float> output[], const std::complex<float> input[], unsigned long n);
        void filterNdec(std::complex<float> output[],
                        const std::complex<float> input[],
                        unsigned long n,
                        unsigned int decimate);

    protected:
        std::vector<float> d_taps;
        unsigned int d_ntaps;
        std::vector<volk::vector<float>> d_aligned_taps;
        volk::vector<std::complex<float>> d_output;
        int d_align;
        int d_naligned;
    };

    class fir_filter_fff
    {
    public:
        fir_filter_fff(int decimation, const std::vector<float> &taps);
        ~fir_filter_fff();

        void set_taps(const std::vector<float> &taps);
        void update_tap(float t, unsigned int index);
        std::vector<float> taps() const;
        unsigned int ntaps() const;

        float filter(const float input[]) const;
        void filterN(float output[], const float input[], unsigned long n);
        void filterNdec(float output[],
                        const float input[],
                        unsigned long n,
                        unsigned int decimate);

    protected:
        std::vector<float> d_taps;
        unsigned int d_ntaps;
        std::vector<volk::vector<float>> d_aligned_taps;
        volk::vector<float> d_output;
        int d_align;
        int d_naligned;
    };
} // namespace libdsp
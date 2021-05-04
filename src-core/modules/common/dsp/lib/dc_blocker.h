#pragma once

#include <complex>
#include <deque>

namespace dsp
{
    class moving_averager_c
    {
    public:
        moving_averager_c(int D);
        ~moving_averager_c();

        std::complex<float> filter(std::complex<float> x);
        std::complex<float> delayed_sig() { return d_out; }

    private:
        int d_length;
        std::complex<float> d_out, d_out_d1, d_out_d2;
        std::deque<std::complex<float>> d_delay_line;
    };

    class DCBlocker
    {
    public:
        DCBlocker(int length, bool long_form);
        ~DCBlocker();
        int work(std::complex<float> *in, int length, std::complex<float> *out);
        int group_delay();

    private:
        int d_length;
        bool d_long_form;
        moving_averager_c *d_ma_0;
        moving_averager_c *d_ma_1;
        moving_averager_c *d_ma_2;
        moving_averager_c *d_ma_3;
        std::deque<std::complex<float>> d_delay_line;
    };
} // namespace libdsp
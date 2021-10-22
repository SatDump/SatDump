#pragma once

#include "block.h"
#include <deque>

/*
Simple DC Blocker from GNU Radio. This is rather slow and 
simply blocks out part of the spectrum, but works for the
purpose.
*/
namespace dsp
{
    class moving_averager_c
    {
    public:
        moving_averager_c(int D);
        ~moving_averager_c();

        complex_t filter(complex_t x);
        complex_t delayed_sig() { return d_out; }

    private:
        int d_length;
        complex_t d_out, d_out_d1, d_out_d2;
        std::deque<complex_t> d_delay_line;
    };

    class DCBlockerBlock : public Block<complex_t, complex_t>
    {
    private:
        int d_length;
        bool d_long_form;
        moving_averager_c *d_ma_0;
        moving_averager_c *d_ma_1;
        moving_averager_c *d_ma_2;
        moving_averager_c *d_ma_3;
        std::deque<complex_t> d_delay_line;
        void work();

    public:
        DCBlockerBlock(std::shared_ptr<dsp::stream<complex_t>> input, int length, bool long_form);
        ~DCBlockerBlock();
    };
}
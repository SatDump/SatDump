#include "dc_blocker.h"

namespace dsp
{

    moving_averager_c::moving_averager_c(int D)
        : d_length(D), d_out(0), d_out_d1(0), d_out_d2(0)
    {
        d_delay_line = std::deque<std::complex<float>>(d_length - 1, std::complex<float>(0, 0));
    }

    moving_averager_c::~moving_averager_c() {}

    std::complex<float> moving_averager_c::filter(std::complex<float> x)
    {
        d_out_d1 = d_out;
        d_delay_line.push_back(x);
        d_out = d_delay_line[0];
        d_delay_line.pop_front();

        std::complex<float> y = x - d_out_d1 + d_out_d2;
        d_out_d2 = y;

        return (y / (float)(d_length));
    }

    DCBlocker::DCBlocker(int length, bool long_form) : d_length(length),
                                                       d_long_form(long_form)
    {
        if (d_long_form)
        {
            d_ma_0 = new moving_averager_c(d_length);
            d_ma_1 = new moving_averager_c(d_length);
            d_ma_2 = new moving_averager_c(d_length);
            d_ma_3 = new moving_averager_c(d_length);
            d_delay_line = std::deque<std::complex<float>>(d_length - 1, std::complex<float>(0, 0));
        }
        else
        {
            d_ma_0 = new moving_averager_c(d_length);
            d_ma_1 = new moving_averager_c(d_length);
            d_ma_2 = NULL;
            d_ma_3 = NULL;
        }
    }

    DCBlocker::~DCBlocker()
    {
        if (d_long_form)
        {
            delete d_ma_0;
            delete d_ma_1;
            delete d_ma_2;
            delete d_ma_3;
        }
        else
        {
            delete d_ma_0;
            delete d_ma_1;
        }
    }

    int DCBlocker::group_delay()
    {
        if (d_long_form)
            return (2 * d_length - 2);
        else
            return d_length - 1;
    }

    int DCBlocker::work(std::complex<float> *in, int length, std::complex<float> *out)
    {

        if (d_long_form)
        {
            std::complex<float> y1, y2, y3, y4, d;
            for (int i = 0; i < length; i++)
            {
                y1 = d_ma_0->filter(in[i]);
                y2 = d_ma_1->filter(y1);
                y3 = d_ma_2->filter(y2);
                y4 = d_ma_3->filter(y3);

                d_delay_line.push_back(d_ma_0->delayed_sig());
                d = d_delay_line[0];
                d_delay_line.pop_front();

                out[i] = d - y4;
            }
        }
        else
        {
            std::complex<float> y1, y2;
            for (int i = 0; i < length; i++)
            {
                y1 = d_ma_0->filter(in[i]);
                y2 = d_ma_1->filter(y1);
                out[i] = d_ma_0->delayed_sig() - y2;
            }
        }

        return length;
    }
} // namespace libdsp
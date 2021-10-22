#include "dc_blocker.h"

namespace dsp
{
    DCBlockerBlock::DCBlockerBlock(std::shared_ptr<dsp::stream<complex_t>> input, int length, bool long_form)
        : Block(input),
          d_length(length),
          d_long_form(long_form)
    {
        if (d_long_form)
        {
            d_ma_0 = new moving_averager_c(d_length);
            d_ma_1 = new moving_averager_c(d_length);
            d_ma_2 = new moving_averager_c(d_length);
            d_ma_3 = new moving_averager_c(d_length);
            d_delay_line = std::deque<complex_t>(d_length - 1, complex_t(0, 0));
        }
        else
        {
            d_ma_0 = new moving_averager_c(d_length);
            d_ma_1 = new moving_averager_c(d_length);
            d_ma_2 = NULL;
            d_ma_3 = NULL;
        }
    }

    DCBlockerBlock::~DCBlockerBlock()
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

    void DCBlockerBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        if (d_long_form)
        {
            complex_t y1, y2, y3, y4, d;
            for (int i = 0; i < nsamples; i++)
            {
                y1 = d_ma_0->filter(input_stream->readBuf[i]);
                y2 = d_ma_1->filter(y1);
                y3 = d_ma_2->filter(y2);
                y4 = d_ma_3->filter(y3);

                d_delay_line.push_back(d_ma_0->delayed_sig());
                d = d_delay_line[0];
                d_delay_line.pop_front();

                output_stream->writeBuf[i] = d - y4;
            }
        }
        else
        {
            complex_t y1, y2;
            for (int i = 0; i < nsamples; i++)
            {
                y1 = d_ma_0->filter(input_stream->readBuf[i]);
                y2 = d_ma_1->filter(y1);
                output_stream->writeBuf[i] = d_ma_0->delayed_sig() - y2;
            }
        }

        input_stream->flush();
        output_stream->swap(nsamples);
    }

    moving_averager_c::moving_averager_c(int D) : d_length(D), d_out(0), d_out_d1(0), d_out_d2(0)
    {
        d_delay_line = std::deque<complex_t>(d_length - 1, complex_t(0, 0));
    }

    moving_averager_c::~moving_averager_c() {}

    complex_t moving_averager_c::filter(complex_t x)
    {
        d_out_d1 = d_out;
        d_delay_line.push_back(x);
        d_out = d_delay_line[0];
        d_delay_line.pop_front();

        complex_t y = x - d_out_d1 + d_out_d2;
        d_out_d2 = y;

        return (y / (float)(d_length));
    }

}
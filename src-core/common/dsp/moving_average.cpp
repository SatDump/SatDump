#include "moving_average.h"

namespace dsp
{
    CCMovingAverageBlock::CCMovingAverageBlock(std::shared_ptr<dsp::stream<std::complex<float>>> input, int length, std::complex<float> scale, int max_iter, unsigned int vlen) : Block(input), d_mov(length, scale, max_iter, vlen)
    {
    }

    void CCMovingAverageBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }
        int d_out = d_mov.work(input_stream->readBuf, nsamples, output_stream->writeBuf);
        input_stream->flush();
        output_stream->swap(d_out);
    }

    FFMovingAverageBlock::FFMovingAverageBlock(std::shared_ptr<dsp::stream<float>> input, int length, float scale, int max_iter, unsigned int vlen) : Block(input), d_mov(length, scale, max_iter, vlen)
    {
    }

    void FFMovingAverageBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }
        int d_out = d_mov.work(input_stream->readBuf, nsamples, output_stream->writeBuf);
        input_stream->flush();
        output_stream->swap(d_out);
    }
}
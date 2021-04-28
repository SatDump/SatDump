#include "rational_resampler.h"

namespace dsp
{
    CCRationalResamplerBlock::CCRationalResamplerBlock(std::shared_ptr<dsp::stream<std::complex<float>>> input, unsigned interpolation, unsigned decimation) : Block(input), d_res(interpolation, decimation)
    {
    }

    void CCRationalResamplerBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
            return;
        int d_out = d_res.work(input_stream->readBuf, nsamples, output_stream->writeBuf);
        input_stream->flush();
        output_stream->swap(d_out);
    }
}
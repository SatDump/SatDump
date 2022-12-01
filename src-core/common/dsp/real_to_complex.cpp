#include "real_to_complex.h"
#include <volk/volk.h>

namespace dsp
{
    RealToComplexBlock::RealToComplexBlock(std::shared_ptr<dsp::stream<float>> input) : Block(input)
    {
    }

    RealToComplexBlock::~RealToComplexBlock()
    {
    }

    void RealToComplexBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        for (int i = 0; i < nsamples; i++)
            output_stream->writeBuf[i] = input_stream->readBuf[i];

        input_stream->flush();
        output_stream->swap(nsamples);
    }
}
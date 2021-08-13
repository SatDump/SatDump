#include "dc_blocker.h"

namespace dsp
{
    DCBlockerBlock::DCBlockerBlock(std::shared_ptr<dsp::stream<std::complex<float>>> input, int length, bool long_form) : Block(input), d_blocker(length, long_form)
    {
    }

    void DCBlockerBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }
        d_blocker.work(input_stream->readBuf, nsamples, output_stream->writeBuf);
        input_stream->flush();
        output_stream->swap(nsamples);
    }
}
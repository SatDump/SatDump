#include "delay_one_imag.h"

namespace dsp
{
    DelayOneImagBlock::DelayOneImagBlock(std::shared_ptr<dsp::stream<std::complex<float>>> input) : Block(input)
    {
    }

    void DelayOneImagBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
            return;
        d_delay.work(input_stream->readBuf, nsamples, output_stream->writeBuf);
        input_stream->flush();
        output_stream->swap(nsamples);
    }
}
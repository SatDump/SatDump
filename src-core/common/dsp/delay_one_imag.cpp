#include "delay_one_imag.h"

namespace dsp
{
    DelayOneImagBlock::DelayOneImagBlock(std::shared_ptr<dsp::stream<complex_t>> input) : Block(input)
    {
        lastSamp = 0;
    }

    void DelayOneImagBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        // Could probably be optimized
        for (int i = 0; i < nsamples; i++)
        {
            float imag = i == 0 ? lastSamp : input_stream->readBuf[i - 1].imag;
            float real = input_stream->readBuf[i].real;
            output_stream->writeBuf[i] = complex_t(real, imag);
        }

        lastSamp = input_stream->readBuf[nsamples - 1].imag;

        input_stream->flush();
        output_stream->swap(nsamples);
    }
}
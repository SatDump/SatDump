#include "correct_iq.h"

namespace dsp
{
    CorrectIQBlock::CorrectIQBlock(std::shared_ptr<dsp::stream<complex_t>> input)
        : Block(input)
    {
    }

    CorrectIQBlock::~CorrectIQBlock()
    {
    }

    void CorrectIQBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        for (int i = 0; i < nsamples; i++)
        {
            acc = acc * beta + input_stream->readBuf[i] * alpha;         // Compute a moving average, of both I & Q
            output_stream->writeBuf[i] = input_stream->readBuf[i] - acc; // Substract it to the input buffer, moving back to 0
        }

        input_stream->flush();
        output_stream->swap(nsamples);
    }
}
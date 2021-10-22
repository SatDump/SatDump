#include "agc.h"

namespace dsp
{
    AGCBlock::AGCBlock(std::shared_ptr<dsp::stream<complex_t>> input, float agc_rate, float reference, float gain, float max_gain)
        : Block(input),
          rate(agc_rate),
          reference(reference),
          gain(gain),
          max_gain(max_gain)
    {
    }

    void AGCBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        for (int i = 0; i < nsamples; i++)
        {
            complex_t output = input_stream->readBuf[i] * gain;

            gain += rate * (reference - sqrt(output.real * output.real +
                                             output.imag * output.imag));

            if (max_gain > 0.0 && gain > max_gain)
                gain = max_gain;

            output_stream->writeBuf[i] = output;
        }

        input_stream->flush();
        output_stream->swap(nsamples);
    }
}
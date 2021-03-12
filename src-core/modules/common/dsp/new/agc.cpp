#include "agc.h"

namespace dsp
{
    AGCBlock::AGCBlock(dsp::stream<std::complex<float>> &input, float agc_rate, float reference, float gain, float max_gain) : Block(input), d_agc(agc_rate, reference, gain, max_gain)
    {
    }

    void AGCBlock::work()
    {
        int nsamples = input_stream.read();
        if (nsamples <= 0)
            return;
        d_agc.work(input_stream.readBuf, nsamples, output_stream.writeBuf);
        input_stream.flush();
        output_stream.swap(nsamples);
    }
}
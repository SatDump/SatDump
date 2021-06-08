#include "pll_carrier_tracking.h"

namespace dsp
{
    PLLCarrierTrackingBlock::PLLCarrierTrackingBlock(std::shared_ptr<dsp::stream<std::complex<float>>> input, float loop_bw, unsigned int min, unsigned int max) : Block(input), d_pll(loop_bw, min, max)
    {
    }

    void PLLCarrierTrackingBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
            return;
        d_pll.work(input_stream->readBuf, nsamples, output_stream->writeBuf);
        input_stream->flush();
        output_stream->swap(nsamples);
    }
}
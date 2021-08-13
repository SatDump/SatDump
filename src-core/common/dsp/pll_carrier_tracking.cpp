#include "pll_carrier_tracking.h"

namespace dsp
{
    PLLCarrierTrackingBlock::PLLCarrierTrackingBlock(std::shared_ptr<dsp::stream<std::complex<float>>> input, float loop_bw, float max, float min) : Block(input), d_pll(loop_bw, max, min)
    {
    }

    void PLLCarrierTrackingBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }
        d_pll.work(input_stream->readBuf, nsamples, output_stream->writeBuf);
        input_stream->flush();
        output_stream->swap(nsamples);
    }
}
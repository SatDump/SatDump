#include "bpsk_carrier_pll.h"

namespace dsp
{
    BPSKCarrierPLLBlock::BPSKCarrierPLLBlock(std::shared_ptr<dsp::stream<std::complex<float>>> input, float alpha, float beta, float max_offset) : Block(input), d_pll(alpha, beta, max_offset)
    {
    }

    void BPSKCarrierPLLBlock::work()
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
#include "costas_loop.h"

namespace dsp
{
    CostasLoopBlock::CostasLoopBlock(std::shared_ptr<dsp::stream<std::complex<float>>> input, float loop_bw, unsigned int order) : Block(input), d_costas(loop_bw, order)
    {
    }

    void CostasLoopBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }
        d_costas.work(input_stream->readBuf, nsamples, output_stream->writeBuf);
        input_stream->flush();
        output_stream->swap(nsamples);
    }
}
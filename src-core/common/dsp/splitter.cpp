#include "splitter.h"

namespace dsp
{
    SplitterBlock::SplitterBlock(std::shared_ptr<dsp::stream<complex_t>> input)
        : Block(input)
    {
        output_stream_2 = std::make_shared<dsp::stream<complex_t>>();
    }

    void SplitterBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        bool can_swap_2nd = true; // output_stream_2->canSwap;

        state_mutex.lock();

        memcpy(output_stream->writeBuf, input_stream->readBuf, nsamples * sizeof(complex_t));
        if (enable_second && can_swap_2nd)
            memcpy(output_stream_2->writeBuf, input_stream->readBuf, nsamples * sizeof(complex_t));

        input_stream->flush();
        output_stream->swap(nsamples);
        if (enable_second && can_swap_2nd)
            output_stream_2->swap(nsamples);

        state_mutex.unlock();
    }
}
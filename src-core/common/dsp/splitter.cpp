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

        state_mutex.lock();

        memcpy(output_stream->writeBuf, input_stream->readBuf, nsamples * sizeof(complex_t));
        if (enable_second)
            memcpy(output_stream_2->writeBuf, input_stream->readBuf, nsamples * sizeof(complex_t));
        if (enable_third)
            memcpy(output_stream_3->writeBuf, input_stream->readBuf, nsamples * sizeof(complex_t));

        input_stream->flush();
        output_stream->swap(nsamples);
        if (enable_second)
            output_stream_2->swap(nsamples);
        if (enable_third)
            output_stream_3->swap(nsamples);

        state_mutex.unlock();
    }
}
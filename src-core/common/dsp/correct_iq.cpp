#include "correct_iq.h"

namespace dsp
{
    template <typename T>
    CorrectIQBlock<T>::CorrectIQBlock(std::shared_ptr<dsp::stream<T>> input, float alpha)
        : Block<T, T>(input)
    {
        beta = 1.0f - alpha;
    }

    template <typename T>
    CorrectIQBlock<T>::~CorrectIQBlock()
    {
    }

    template <typename T>
    void CorrectIQBlock<T>::work()
    {
        int nsamples = Block<T, T>::input_stream->read();
        if (nsamples <= 0)
        {
            Block<T, T>::input_stream->flush();
            return;
        }

        for (int i = 0; i < nsamples; i++)
        {
            acc = acc * beta + Block<T, T>::input_stream->readBuf[i] * alpha;                      // Compute a moving average, of both I & Q
            Block<T, T>::output_stream->writeBuf[i] = Block<T, T>::input_stream->readBuf[i] - acc; // Substract it to the input buffer, moving back to 0
        }

        Block<T, T>::input_stream->flush();
        Block<T, T>::output_stream->swap(nsamples);
    }

    template class CorrectIQBlock<complex_t>;
    template class CorrectIQBlock<float>;
}
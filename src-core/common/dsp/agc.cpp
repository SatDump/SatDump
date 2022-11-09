#include "agc.h"

namespace dsp
{
    template <typename T>
    AGCBlock<T>::AGCBlock(std::shared_ptr<dsp::stream<T>> input, float agc_rate, float reference, float gain, float max_gain)
        : Block<T, T>(input),
          rate(agc_rate),
          reference(reference),
          gain(gain),
          max_gain(max_gain)
    {
    }

    template <typename T>
    void AGCBlock<T>::work()
    {
        int nsamples = Block<T, T>::input_stream->read();
        if (nsamples <= 0)
        {
            Block<T, T>::input_stream->flush();
            return;
        }

        for (int i = 0; i < nsamples; i++)
        {
            T output = Block<T, T>::input_stream->readBuf[i] * gain;

            if constexpr (std::is_same_v<T, float>)
                gain += rate * (reference - fabsf(output));
            if constexpr (std::is_same_v<T, complex_t>)
                gain += rate * (reference - sqrt(output.real * output.real +
                                                 output.imag * output.imag));

            if (max_gain > 0.0 && gain > max_gain)
                gain = max_gain;

            Block<T, T>::output_stream->writeBuf[i] = output;
        }

        Block<T, T>::input_stream->flush();
        Block<T, T>::output_stream->swap(nsamples);
    }

    template class AGCBlock<complex_t>;
    template class AGCBlock<float>;
}
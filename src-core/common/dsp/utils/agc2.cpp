#include "agc2.h"

namespace dsp
{
    template <typename T>
    AGC2Block<T>::AGC2Block(std::shared_ptr<dsp::stream<T>> input, float target, float bias, float gain)
        : Block<T, T>(input),
          d_target(target), d_bias(bias), d_gain(gain)
    {
    }

    template <typename T>
    void AGC2Block<T>::work()
    {
        int nsamples = Block<T, T>::input_stream->read();
        if (nsamples <= 0)
        {
            Block<T, T>::input_stream->flush();
            return;
        }

        for (int i = 0; i < nsamples; i++)
        {
            T sample = Block<T, T>::input_stream->readBuf[i];

            if constexpr (std::is_same_v<T, float>)
            {
                sample -= bias;                                                  // Apply current DC bias
                bias = bias * (1 - d_bias) + sample * d_bias;                    // Calculate new DC bias
                gain = d_target / moving_avg;                                    // Calculate new gain
                moving_avg = moving_avg * (1 - d_gain) + fabsf(sample) * d_gain; // Moving average to calculate gain
                sample *= gain;                                                  // Apply gain
            }
            if constexpr (std::is_same_v<T, complex_t>)
            {
            }

            Block<T, T>::output_stream->writeBuf[i] = sample;
        }

        Block<T, T>::input_stream->flush();
        Block<T, T>::output_stream->swap(nsamples);
    }

    template class AGC2Block<complex_t>;
    template class AGC2Block<float>;
}
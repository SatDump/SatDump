#include "common/dsp/filter/deemphasis.h"
#include "common/dsp/buffer.h"
#include <complex.h>
#include <memory>
#include <type_traits>

namespace dsp
{
    template<typename T>
    DeEmphasisBlock<T>::DeEmphasisBlock(std::shared_ptr<dsp::stream<T>> input, double quad_sampl_rate, double tau) : Block<T, T>(input)
    {
        // Init buffer
        buffer = create_volk_buffer<T>(2 * STREAM_BUFFER_SIZE);

        float dt = 1.0f / quad_sampl_rate;
        alpha = dt / (tau + dt);
    }

    template <typename T>
    DeEmphasisBlock<T>::~DeEmphasisBlock()
    {
        volk_free(buffer);
    }

    template <typename T>
    void DeEmphasisBlock<T>::work()
    {
        int nsamples = Block<T, T>::input_stream->read();
        if (nsamples <= 0)
        {
            Block<T, T>::input_stream->flush();
            return;
        }

        memcpy(&buffer[in_buffer], Block<T, T>::input_stream->readBuf, nsamples * sizeof(T));
        in_buffer += nsamples;

        int consumed = 0;
        for (int i = 0; (i + d_nsamples) < in_buffer; i += d_nsamples)
        {
            T input = Block<T, T>::input_stream->readBuf[i];
            T output = Block<T, T>::output_stream->readBuf[i];

            consumed += d_nsamples;

            //float dt = 1.0f / quad_sampl_rate;
            //alpha = dt / (tau + dt);

            if constexpr (std::is_same_v<T, float>)
            {
                std::vector<float> input;
                std::vector<float> output;
                output[0] = (alpha * input[0]) + ((1 - alpha) * lastSamp);
                for (int j = 1; j < d_nsamples; j++)
                {
                    output[j] = (alpha * input[j]) + ((1 - alpha) * output[j -1]);
                }
                lastSamp = output[d_nsamples - 1];
            }

            if constexpr (std::is_same_v<T, complex_t>)
            {
                std::vector<complex_t> input;
                std::vector<complex_t> output;
                output[0].imag = (alpha * input[0].imag) + ((1 - alpha) * lastSampImag);
                output[0].real = (alpha * input[0].real) + ((1 - alpha) * lastSampReal);
                for (int j = 1; j < d_nsamples; j++)
                {
                    output[j].imag = (alpha * input[j].imag) + ((1 - alpha) * output[j - 1].imag);
                    output[j].real = (alpha * input[j].real) + ((1 - alpha) * output[j - 1].real);
                }

                lastSampImag = output[d_nsamples - 1].imag;
                lastSampReal = output[d_nsamples - 1].real;

            }

            Block<T, T>::output_stream->writeBuf[i] = output;
        }

        Block<T, T>::input_stream->flush();
        //Block<T, T>::output_stream->swap(nsamples);
        Block<T, T>::output_stream->swap(consumed);

        memmove(&buffer[0], &buffer[consumed], (in_buffer - consumed) * sizeof(T));
        in_buffer -= consumed;

    }

    template class DeEmphasisBlock<complex_t>;
    template class DeEmphasisBlock<float>;
}

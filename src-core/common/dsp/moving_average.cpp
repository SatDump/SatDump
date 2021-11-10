#include "moving_average.h"
#include <numeric>

// Older systems running livolk1 lack this kernel... So provide our own if that's the case
#ifdef VOLK_NO_volk_32fc_x2_add_32fc
void volk_32fc_x2_add_32fc(lv_32fc_t *cVector,
                           const lv_32fc_t *aVector,
                           const lv_32fc_t *bVector,
                           unsigned int num_points)
{
    lv_32fc_t *cPtr = cVector;
    const lv_32fc_t *aPtr = aVector;
    const lv_32fc_t *bPtr = bVector;
    unsigned int number = 0;

    for (number = 0; number < num_points; number++)
    {
        *cPtr++ = (*aPtr++) + (*bPtr++);
    }
}
#endif

namespace dsp
{
    CCMovingAverageBlock::CCMovingAverageBlock(std::shared_ptr<dsp::stream<complex_t>> input, int length, complex_t scale, int max_iter, unsigned int vlen)
        : Block(input),
          d_length(length),
          d_scale(scale),
          d_max_iter(max_iter),
          d_vlen(vlen)
    {
        // Get alignement parameters
        int align = volk_get_alignment();

        // Buffer
        in_buffer = 0;
        int size = 2 * STREAM_BUFFER_SIZE;
        buffer = (complex_t *)volk_malloc(size * sizeof(complex_t), align);
        std::fill(buffer, &buffer[size], 0);

        // Init vectors
        if (d_vlen > 1)
        {
            d_sum = std::vector<complex_t>(d_vlen);
            d_scales = std::vector<complex_t>(d_vlen, d_scale);
        }
    }

    CCMovingAverageBlock::~CCMovingAverageBlock()
    {
        volk_free(buffer);
    }

    void CCMovingAverageBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        memcpy(&buffer[in_buffer], input_stream->readBuf, nsamples * sizeof(complex_t));
        in_buffer += nsamples;

        // Compute how many iterations we should go through
        const unsigned int num_iter = std::min(in_buffer, d_max_iter);

        if (d_vlen == 1) // Slower way, but for vlen = 1
        {
            complex_t sum = std::accumulate(&buffer[0], &buffer[d_length - 1], complex_t(0, 0));

            for (unsigned int i = 0; i < num_iter; i++)
            {
                sum += buffer[i + d_length - 1];
                output_stream->writeBuf[i] = sum * d_scale;
                sum -= buffer[i];
            }
        }
        else // This only works with d_vlen > 1
        {
            std::copy(&buffer[0], &buffer[d_vlen], std::begin(d_sum));

            for (int i = 1; i < d_length - 1; i++)
                volk_32fc_x2_add_32fc((lv_32fc_t *)d_sum.data(), (lv_32fc_t *)d_sum.data(), (lv_32fc_t *)&buffer[i * d_vlen], d_vlen);

            for (unsigned int i = 0; i < num_iter; i++)
            {
                volk_32fc_x2_add_32fc((lv_32fc_t *)d_sum.data(), (lv_32fc_t *)d_sum.data(), (lv_32fc_t *)&buffer[(i + d_length - 1) * d_vlen], d_vlen);
                volk_32fc_x2_multiply_32fc((lv_32fc_t *)&output_stream->writeBuf[i * d_vlen], (lv_32fc_t *)d_sum.data(), (lv_32fc_t *)d_scales.data(), d_vlen);
                for (unsigned int elem = 0; elem < d_vlen; elem++)
                    d_sum.data()[elem] -= (&buffer[i * d_vlen])[elem];
            }
        }

        // Keep the last few samples for averaging
        if (in_buffer - d_length > 0)
        {
            memmove(&buffer[0], &buffer[in_buffer - d_length], d_length * sizeof(float));
            in_buffer = d_length;
        }

        input_stream->flush();
        output_stream->swap(num_iter);
    }

    FFMovingAverageBlock::FFMovingAverageBlock(std::shared_ptr<dsp::stream<float>> input, int length, float scale, int max_iter, unsigned int vlen)
        : Block(input),
          d_length(length),
          d_scale(scale),
          d_max_iter(max_iter),
          d_vlen(vlen)
    {
        // Get alignement parameters
        int align = volk_get_alignment();

        // Buffer
        in_buffer = 0;
        int size = 2 * STREAM_BUFFER_SIZE;
        buffer = (float *)volk_malloc(size * sizeof(float), align);
        std::fill(buffer, &buffer[size], 0);

        // Init vectors
        if (d_vlen > 1)
        {
            d_sum = std::vector<float>(d_vlen);
            d_scales = std::vector<float>(d_vlen, d_scale);
        }
    }

    FFMovingAverageBlock::~FFMovingAverageBlock()
    {
        volk_free(buffer);
    }

    void FFMovingAverageBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        memcpy(&buffer[in_buffer], input_stream->readBuf, nsamples * sizeof(float));
        in_buffer += nsamples;

        // Compute how many iterations we should go through
        const unsigned int num_iter = std::min(in_buffer, d_max_iter);

        if (d_vlen == 1) // Slower way, but for vlen = 1
        {
            float sum = std::accumulate(&buffer[0], &buffer[d_length - 1], float());

            for (unsigned int i = 0; i < num_iter; i++)
            {
                sum += buffer[i + d_length - 1];
                output_stream->writeBuf[i] = sum * d_scale;
                sum -= buffer[i];
            }
        }
        else // This only works with d_vlen > 1
        {
            std::copy(&buffer[0], &buffer[d_vlen], std::begin(d_sum));

            for (int i = 1; i < d_length - 1; i++)
                volk_32f_x2_add_32f(d_sum.data(), d_sum.data(), &buffer[i * d_vlen], d_vlen);

            for (unsigned int i = 0; i < num_iter; i++)
            {
                volk_32f_x2_add_32f(d_sum.data(), d_sum.data(), &buffer[(i + d_length - 1) * d_vlen], d_vlen);
                volk_32f_x2_multiply_32f(&output_stream->writeBuf[i * d_vlen], d_sum.data(), d_scales.data(), d_vlen);
                volk_32f_x2_subtract_32f(d_sum.data(), d_sum.data(), &buffer[i * d_vlen], d_vlen);
            }
        }

        // Some history required
        if (in_buffer - d_length > 0)
        {
            memmove(&buffer[0], &buffer[in_buffer - d_length], d_length * sizeof(float));
            in_buffer = d_length;
        }

        input_stream->flush();
        output_stream->swap(num_iter);
    }
}
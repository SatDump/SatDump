#include "rational_resampler.h"
#include <numeric>
#include "firdes.h"

namespace dsp
{
    template <typename T>
    RationalResamplerBlock<T>::RationalResamplerBlock(std::shared_ptr<dsp::stream<T>> input, unsigned interpolation, unsigned decimation, std::vector<float> custom_taps)
        : Block<T, T>(input),
          d_interpolation(interpolation),
          d_decimation(decimation),
          d_ctr(0)
    {
        // Get alignement parameters
        int align = volk_get_alignment();

        // Buffer
        in_buffer = 0;
        int size = 2 * STREAM_BUFFER_SIZE;
        buffer = (T *)volk_malloc(size * sizeof(T), align);
        std::fill(buffer, &buffer[size], 0);

        // Start by reducing the interp and decim by their GCD
        int gcd = std::gcd(interpolation, decimation);
        d_interpolation /= gcd;
        d_decimation /= gcd;

        // Generate taps
        std::vector<float> rtaps = custom_taps.size() > 0 ? custom_taps : firdes::design_resampler_filter_float(d_interpolation, d_decimation, 0.4); // 0.4 = Fractional BW

        // Filter number & tap number
        nfilt = d_interpolation;
        ntaps = rtaps.size() / nfilt;

        // If Ntaps is slightly over 1, add 1 tap
        if (fmod(double(rtaps.size()) / double(nfilt), 1.0) > 0.0)
            ntaps++;

        // Init tap buffers
        taps = (float **)volk_malloc(nfilt * sizeof(float *), align);
        for (int i = 0; i < nfilt; i++)
        {
            this->taps[i] = (float *)volk_malloc(ntaps * sizeof(float), align);
            memset(this->taps[i], 0, ntaps);
        }

        // Setup taps
        for (int i = 0; i < (int)rtaps.size(); i++)
            taps[i % nfilt][(ntaps - 1) - (i / nfilt)] = rtaps[i];
    }

    template <typename T>
    RationalResamplerBlock<T>::~RationalResamplerBlock()
    {
        for (int i = 0; i < nfilt; i++)
            volk_free(taps[i]);
        volk_free(taps);
        volk_free(buffer);
    }

    template <typename T>
    void RationalResamplerBlock<T>::work()
    {
        int nsamples = Block<T, T>::input_stream->read();
        if (nsamples <= 0)
        {
            Block<T, T>::input_stream->flush();
            return;
        }

        memcpy(&buffer[ntaps], Block<T, T>::input_stream->readBuf, nsamples * sizeof(T));
        in_buffer = ntaps + nsamples;
        Block<T, T>::input_stream->flush();

        int outc = 0;
        for (int i = 0; i < in_buffer - ntaps;)
        {
            if constexpr (std::is_same_v<T, float>)
                volk_32f_x2_dot_prod_32f(&Block<T, T>::output_stream->writeBuf[outc++], &buffer[i], taps[d_ctr], ntaps);
            if constexpr (std::is_same_v<T, complex_t>)
                volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&Block<T, T>::output_stream->writeBuf[outc++], (lv_32fc_t *)&buffer[i], taps[d_ctr], ntaps);
            d_ctr += this->d_decimation;
            while (d_ctr >= d_interpolation)
            {
                d_ctr -= d_interpolation;
                i++;
            }
        }

        memmove(&buffer[0], &buffer[in_buffer - ntaps], ntaps * sizeof(T));

        Block<T, T>::output_stream->swap(outc);
    }

    template class RationalResamplerBlock<complex_t>;
    template class RationalResamplerBlock<float>;
}

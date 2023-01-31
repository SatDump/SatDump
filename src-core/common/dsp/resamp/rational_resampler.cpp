#include "rational_resampler.h"
#include <numeric>
#include "common/dsp/filter/firdes.h"

namespace dsp
{
    template <typename T>
    RationalResamplerBlock<T>::RationalResamplerBlock(std::shared_ptr<dsp::stream<T>> input, unsigned interpolation, unsigned decimation, std::vector<float> custom_taps)
        : Block<T, T>(input),
          d_interpolation(interpolation),
          d_decimation(decimation),
          d_ctr(0)
    {
        // Buffer
        buffer = create_volk_buffer<T>(2 * STREAM_BUFFER_SIZE);

        // Start by reducing the interp and decim by their GCD
        int gcd = std::gcd(interpolation, decimation);
        d_interpolation /= gcd;
        d_decimation /= gcd;

        // Generate taps
        std::vector<float> rtaps = custom_taps.size() > 0 ? custom_taps : firdes::design_resampler_filter_float(d_interpolation, d_decimation, 0.4); // 0.4 = Fractional BW

        pfb.init(rtaps, d_interpolation);
    }

    template <typename T>
    RationalResamplerBlock<T>::~RationalResamplerBlock()
    {
        volk_free(buffer);
    }

    template <typename T>
    int RationalResamplerBlock<T>::process(T *input, int nsamples, T *output)
    {
        memcpy(&buffer[pfb.ntaps - 1], input, nsamples * sizeof(T));

        outc = 0;
        while (inc < nsamples)
        {
            if constexpr (std::is_same_v<T, float>)
                volk_32f_x2_dot_prod_32f(&output[outc++], &buffer[inc], pfb.taps[d_ctr], pfb.ntaps);
            if constexpr (std::is_same_v<T, complex_t>)
                volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&output[outc++], (lv_32fc_t *)&buffer[inc], pfb.taps[d_ctr], pfb.ntaps);
            d_ctr += this->d_decimation;
            inc += d_ctr / d_interpolation;
            d_ctr = d_ctr % d_interpolation;
        }

        inc -= nsamples;

        memmove(&buffer[0], &buffer[nsamples], pfb.ntaps * sizeof(T));

        return outc;
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

        int outn = process(Block<T, T>::input_stream->readBuf, nsamples, Block<T, T>::output_stream->writeBuf);

        Block<T, T>::input_stream->flush();
        Block<T, T>::output_stream->swap(outn);
    }

    template class RationalResamplerBlock<complex_t>;
    template class RationalResamplerBlock<float>;
}

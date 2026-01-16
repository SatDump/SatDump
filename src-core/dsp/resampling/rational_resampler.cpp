#include "rational_resampler.h"
#include "common/dsp/block.h"
#include "common/dsp/filter/firdes.h"
#include "common/dsp/window/window.h"
#include <cstdint>

#define BRANCHLESS_CLIP(x, clip) (0.5 * (std::abs(x + clip) - std::abs(x - clip)))

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        RationalResamplerBlock<T>::RationalResamplerBlock()
            : BlockSimple<T, T>(std::is_same_v<T, complex_t> ? "rational_resampler_cc" : "rational_resampler_ff", {{"in", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}},
                                {{"out", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}})
        {
            // Buffer
            if (buffer == nullptr)
                volk_free(buffer); // TODOREWORK

            buffer = dsp::create_volk_buffer<T>(1e6);
        }

        template <typename T>
        void RationalResamplerBlock<T>::init()
        {
            d_interpolation = p_interpolation;
            d_decimation = p_decimation;

            // Start by reducing the interp and decim by their GCD
            int gcd = std::gcd(d_interpolation, d_decimation);
            d_interpolation /= gcd;
            d_decimation /= gcd;

            // Generate taps
            std::vector<float> rtaps = p_taps.size() > 0 ? p_taps : dsp::firdes::design_resampler_filter_float(d_interpolation, d_decimation, 0.4); // 0.4 = Fractional BW
            pfb.init(rtaps, d_interpolation);

            // Set size ratio for output buffer
            double v = double(d_interpolation) / double(d_decimation);
            if (v > 1)
                BlockSimple<T, T>::output_buffer_size_ratio = v * 1.5;
        }

        template <typename T>
        RationalResamplerBlock<T>::~RationalResamplerBlock()
        {
            if (buffer == nullptr)
                volk_free(buffer);
        }

        template <typename T>
        uint32_t RationalResamplerBlock<T>::process(T *input, uint32_t nsamples, T *output)
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

        template class RationalResamplerBlock<complex_t>;
        template class RationalResamplerBlock<float>;
    } // namespace ndsp
} // namespace satdump
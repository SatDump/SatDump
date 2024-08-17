#include "rational_resampler.h"
#include "common/dsp/buffer.h"

#include "common/dsp/filter/firdes.h"

namespace ndsp
{
    template <typename T>
    RationalResampler<T>::RationalResampler()
        : ndsp::Block(std::is_same_v<T, float> ? "rational_resampler_ff" : "rational_resampler_cc", {{sizeof(T)}}, {{sizeof(T)}})
    {
    }

    template <typename T>
    RationalResampler<T>::~RationalResampler()
    {
        if (buffer != nullptr)
            volk_free(buffer);
    }

    template <typename T>
    void RationalResampler<T>::start()
    {
        set_params();
        ndsp::buf::init_nafifo_stdbuf<T>(outputs[0], 2, ((ndsp::buf::StdBuf<T> *)inputs[0]->write_buf())->max * 4); // TODO FIX
        ndsp::Block::start();
    }

    template <typename T>
    void RationalResampler<T>::set_params(nlohmann::json p)
    {
        if (p.contains("d_interpolation"))
            interpolation = p["d_interpolation"];
        if (p.contains("d_decimation"))
            decimation = p["d_decimation"];

        this->interpolation = d_interpolation;
        this->decimation = d_decimation;
        d_ctr = 0;

        // Buffer
        buffer = dsp::create_volk_buffer<T>(((ndsp::buf::StdBuf<T> *)inputs[0]->write_buf())->max * 2);
    }

    template <typename T>
    void RationalResampler<T>::set_ratio(unsigned interpolation, unsigned decimation, std::vector<float> custom_taps)
    {
        this->interpolation = interpolation;
        this->decimation = decimation;

        // Start by reducing the interp and decim by their GCD
        int gcd = std::gcd(interpolation, decimation);
        this->interpolation /= gcd;
        this->decimation /= gcd;

        // Generate taps
        std::vector<float> rtaps = custom_taps.size() > 0 ? custom_taps : dsp::firdes::design_resampler_filter_float(this->interpolation, this->decimation, 0.4); // 0.4 = Fractional BW
        pfb.init(rtaps, this->interpolation);
    }

    template <typename T>
    int RationalResampler<T>::process(T *input, int nsamples, T *output)
    {
        memcpy(&buffer[pfb.ntaps - 1], input, nsamples * sizeof(T));

        outc = 0;
        while (inc < nsamples)
        {
            if constexpr (std::is_same_v<T, float>)
                volk_32f_x2_dot_prod_32f(&output[outc++], &buffer[inc], pfb.taps[d_ctr], pfb.ntaps);
            if constexpr (std::is_same_v<T, complex_t>)
                volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&output[outc++], (lv_32fc_t *)&buffer[inc], pfb.taps[d_ctr], pfb.ntaps);
            d_ctr += this->decimation;
            inc += d_ctr / this->interpolation;
            d_ctr = d_ctr % this->interpolation;
        }

        inc -= nsamples;

        memmove(&buffer[0], &buffer[nsamples], pfb.ntaps * sizeof(T));

        return outc;
    }

    template <typename T>
    void RationalResampler<T>::work()
    {
        if (!inputs[0]->read())
        {
            auto *rbuf = (ndsp::buf::StdBuf<T> *)inputs[0]->read_buf();
            auto *wbuf = (ndsp::buf::StdBuf<T> *)outputs[0]->write_buf();

            wbuf->cnt = process(rbuf->dat, rbuf->cnt, wbuf->dat);

            inputs[0]->flush();
            outputs[0]->write();
        }
    }

    template class RationalResampler<complex_t>;
    template class RationalResampler<float>;
}

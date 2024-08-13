#include "fir.h"
#include "common/dsp/buffer.h"

namespace ndsp
{
    template <typename T>
    FIRFilter<T>::FIRFilter()
        : ndsp::Block("fir_filter", {{sizeof(T)}}, {{sizeof(T)}})
    {
    }

    template <typename T>
    FIRFilter<T>::~FIRFilter()
    {
        if (taps != nullptr)
        {
            for (int i = 0; i < aligned_tap_count; i++)
                volk_free(taps[i]);
            volk_free(taps);
        }
        if (buffer != nullptr)
            volk_free(buffer);
    }

    template <typename T>
    void FIRFilter<T>::start()
    {
        set_params();
        ndsp::buf::init_nafifo_stdbuf<T>(outputs[0], 2, ((ndsp::buf::StdBuf<T> *)inputs[0]->read_buf())->max);
        ndsp::Block::start();
    }

    template <typename T>
    void FIRFilter<T>::set_params(nlohmann::json p)
    {
        if (p.contains("taps"))
            d_taps = p["taps"].get<std::vector<float>>();

        // Get alignement parameters
        align = volk_get_alignment();
        aligned_tap_count = std::max<int>(1, align / sizeof(T));

        // Set taps
        ntaps = d_taps.size();

        // Init taps
        this->taps = (float **)volk_malloc(aligned_tap_count * sizeof(float *), align);
        for (int i = 0; i < aligned_tap_count; i++)
        {
            this->taps[i] = (float *)volk_malloc((ntaps + aligned_tap_count - 1) * sizeof(float), align);
            for (int y = 0; y < ntaps + aligned_tap_count - 1; y++)
                this->taps[i][y] = 0;
            for (int j = 0; j < ntaps; j++)
                this->taps[i][i + j] = d_taps[(ntaps - 1) - j]; // Reverse taps
        }

        // Init buffer
        buffer = dsp::create_volk_buffer<T>(((ndsp::buf::StdBuf<T> *)inputs[0]->read_buf())->max * 2);
    }

    template <typename T>
    void FIRFilter<T>::work()
    {
        if (!inputs[0]->read())
        {
            auto *rbuf = (ndsp::buf::StdBuf<T> *)inputs[0]->read_buf();
            auto *wbuf = (ndsp::buf::StdBuf<T> *)outputs[0]->write_buf();

            int nsamples = rbuf->cnt;
            memcpy(&buffer[ntaps], rbuf->dat, nsamples * sizeof(T));
            inputs[0]->flush();

            if constexpr (std::is_same_v<T, float>)
            {
                for (int i = 0; i < nsamples; i++)
                {
                    // Doing it this way instead of the normal :
                    // volk_32f_x2_dot_prod_32f(&output_stream->writeBuf[i], &buffer[i + 1], taps, ntaps);
                    // turns out to enhance performances quite a bit... Initially tested this after noticing
                    // inconsistencies between the above simple way and GNU Radio's implementation
                    const float *ar = (float *)((size_t)&buffer[i + 1] & ~(align - 1));
                    const unsigned al = &buffer[i + 1] - ar;
                    volk_32f_x2_dot_prod_32f_a(&wbuf->dat[i], ar, taps[al], ntaps + al);
                }
            }
            if constexpr (std::is_same_v<T, complex_t>)
            {
                for (int i = 0; i < nsamples; i++)
                {
                    // Doing it this way instead of the normal :
                    // volk_32fc_32f_dot_prod_32fc(&output_stream->writeBuf[i], &buffer[i + 1], taps, ntaps);
                    // turns out to enhance performances quite a bit... Initially tested this after noticing
                    // inconsistencies between the above simple way and GNU Radio's implementation
                    const complex_t *ar = (complex_t *)((size_t)&buffer[i + 1] & ~(align - 1));
                    const unsigned al = &buffer[i + 1] - ar;
                    volk_32fc_32f_dot_prod_32fc_a((lv_32fc_t *)&wbuf->dat[i], (lv_32fc_t *)ar, taps[al], ntaps + al);
                }
            }

            wbuf->cnt = nsamples;
            outputs[0]->write();

            memmove(&buffer[0], &buffer[nsamples], ntaps * sizeof(T));
        }
    }

    template class FIRFilter<complex_t>;
    template class FIRFilter<float>;
}

#include "agc.h"
#include "common/dsp/buffer.h"

namespace ndsp
{
    template <typename T>
    Agc<T>::Agc()
        : ndsp::Block(std::is_same_v<T, float> ? "agc_ff" : "agc_cc", {{sizeof(T)}}, {{sizeof(T)}})
    {
    }

    template <typename T>
    Agc<T>::~Agc()
    {
    }

    template <typename T>
    void Agc<T>::start()
    {
        set_params();
        ndsp::buf::init_nafifo_stdbuf<T>(outputs[0], 2, ((ndsp::buf::StdBuf<T> *)inputs[0]->read_buf())->max);
        ndsp::Block::start();
    }

    template <typename T>
    void Agc<T>::set_params(nlohmann::json p)
    {
        if (p.contains("rate"))
            d_rate = p["rate"];
        if (p.contains("reference"))
            d_reference = p["reference"];
        if (p.contains("gain"))
            d_gain = p["gain"];
        if (p.contains("max_gain"))
            d_max_gain = p["max_gain"];

        rate = d_rate;
        reference = d_reference;
        gain = d_gain;
        max_gain = d_max_gain;
    }

    template <typename T>
    void Agc<T>::work()
    {
        if (!inputs[0]->read())
        {
            auto *rbuf = (ndsp::buf::StdBuf<T> *)inputs[0]->read_buf();
            auto *wbuf = (ndsp::buf::StdBuf<T> *)outputs[0]->write_buf();

            for (int i = 0; i < rbuf->cnt; i++)
            {
                T output = rbuf->dat[i] * gain;

                if constexpr (std::is_same_v<T, float>)
                    gain += rate * (reference - fabsf(output));
                if constexpr (std::is_same_v<T, complex_t>)
                    gain += rate * (reference - sqrt(output.real * output.real +
                                                     output.imag * output.imag));

                if (max_gain > 0.0 && gain > max_gain)
                    gain = max_gain;

                wbuf->dat[i] = output;
            }

            inputs[0]->flush();
            wbuf->cnt = rbuf->cnt;
            outputs[0]->write();
        }
    }

    template class Agc<complex_t>;
    template class Agc<float>;
}

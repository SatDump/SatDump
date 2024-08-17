#include "correct_iq.h"
#include "common/dsp/buffer.h"

namespace ndsp
{
    template <typename T>
    CorrectIQ<T>::CorrectIQ()
        : ndsp::Block(std::is_same_v<T, float> ? "correct_iq_ff" : "correct_iq_cc", {{sizeof(T)}}, {{sizeof(T)}})
    {
    }

    template <typename T>
    CorrectIQ<T>::~CorrectIQ()
    {
    }

    template <typename T>
    void CorrectIQ<T>::start()
    {
        set_params();
        ndsp::buf::init_nafifo_stdbuf<T>(outputs[0], 2, ((ndsp::buf::StdBuf<T> *)inputs[0]->write_buf())->max);
        ndsp::Block::start();
    }

    template <typename T>
    void CorrectIQ<T>::set_params(nlohmann::json p)
    {
        if (p.contains("alpha"))
            d_alpha = p["alpha"];

        alpha = d_alpha;

        beta = 1.0f - alpha;
    }

    template <typename T>
    void CorrectIQ<T>::work()
    {
        if (!inputs[0]->read())
        {
            auto *rbuf = (ndsp::buf::StdBuf<T> *)inputs[0]->read_buf();
            auto *wbuf = (ndsp::buf::StdBuf<T> *)outputs[0]->write_buf();

            for (int i = 0; i < rbuf->cnt; i++)
            {
                acc = acc * beta + rbuf->dat[i] * alpha; // Compute a moving average, of both I & Q
                wbuf->dat[i] = rbuf->dat[i] - acc;       // Substract it to the input buffer, moving back to 0
            }

            inputs[0]->flush();
            wbuf->cnt = rbuf->cnt;
            outputs[0]->write();
        }
    }

    template class CorrectIQ<complex_t>;
    template class CorrectIQ<float>;
}

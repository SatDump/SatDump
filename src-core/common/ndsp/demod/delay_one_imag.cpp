#include "delay_one_imag.h"
#include "common/dsp/block.h"

namespace ndsp
{
    DelayOneImag::DelayOneImag()
        : ndsp::Block("delay_one_imag", {{sizeof(complex_t)}}, {{sizeof(complex_t)}})
    {
    }

    void DelayOneImag::start()
    {
        set_params();
        ndsp::buf::init_nafifo_stdbuf<complex_t>(outputs[0], 2, ((ndsp::buf::StdBuf<complex_t> *)inputs[0]->write_buf())->max);
        ndsp::Block::start();
    }

    void DelayOneImag::set_params(nlohmann::json p)
    {
    }

    void DelayOneImag::work()
    {
        if (!inputs[0]->read())
        {
            auto *rbuf = (ndsp::buf::StdBuf<complex_t> *)inputs[0]->read_buf();
            auto *wbuf = (ndsp::buf::StdBuf<complex_t> *)outputs[0]->write_buf();

            // Could probably be optimized
            for (int i = 0; i < rbuf->cnt; i++)
            {
                float imag = i == 0 ? lastSamp : rbuf->dat[i - 1].imag;
                float real = rbuf->dat[i].real;
                wbuf->dat[i] = complex_t(real, imag);
            }

            lastSamp = rbuf->dat[rbuf->cnt - 1].imag;

            wbuf->cnt = rbuf->cnt;

            outputs[0]->write();
            inputs[0]->flush();
        }
    }
}

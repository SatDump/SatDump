#include "pm_to_bpsk.h"
#include "common/dsp/block.h"

namespace ndsp
{
    PMToBPSK::PMToBPSK()
        : ndsp::Block("pm_to_bpsk", {{sizeof(complex_t)}}, {{sizeof(complex_t)}})
    {
    }

    void PMToBPSK::start()
    {
        set_params();
        ndsp::buf::init_nafifo_stdbuf<complex_t>(outputs[0], 2, ((ndsp::buf::StdBuf<complex_t> *)inputs[0]->write_buf())->max);
        ndsp::Block::start();
    }

    void PMToBPSK::set_params(nlohmann::json p)
    {
        if (p.contains("samplerate"))
            d_samplerate = p["samplerate"];
        if (p.contains("symbolrate"))
            d_symbolrate = p["symbolrate"];

        phase_delta = complex_t(cos(dsp::hz_to_rad(-d_symbolrate, d_samplerate)), sin(dsp::hz_to_rad(-d_symbolrate, d_samplerate)));
        phase = complex_t(1, 0);
    }

    void PMToBPSK::work()
    {
        if (!inputs[0]->read())
        {
            auto *rbuf = (ndsp::buf::StdBuf<complex_t> *)inputs[0]->read_buf();
            auto *wbuf = (ndsp::buf::StdBuf<complex_t> *)outputs[0]->write_buf();

            for (int i = 0; i < rbuf->cnt; i++)
                rbuf->dat[i] = complex_t(0, rbuf->dat[i].imag);

#if VOLK_VERSION >= 030100
            volk_32fc_s32fc_x2_rotator2_32fc((lv_32fc_t *)wbuf->dat, (lv_32fc_t *)rbuf->dat, (lv_32fc_t *)&phase_delta, (lv_32fc_t *)&phase, rbuf->cnt);
#else
            volk_32fc_s32fc_x2_rotator_32fc((lv_32fc_t *)wbuf->dat, (lv_32fc_t *)rbuf->dat, phase_delta, (lv_32fc_t *)&phase, rbuf->cnt);
#endif

            wbuf->cnt = rbuf->cnt;

            outputs[0]->write();
            inputs[0]->flush();
        }
    }
}

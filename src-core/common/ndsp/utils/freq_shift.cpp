#include "freq_shift.h"
#include "common/dsp/block.h"

namespace ndsp
{
    FreqShift::FreqShift()
        : ndsp::Block("freq_shift", {{sizeof(complex_t)}}, {{sizeof(complex_t)}})
    {
    }

    void FreqShift::start()
    {
        set_params();
        ndsp::buf::init_nafifo_stdbuf<complex_t>(outputs[0], 2, ((ndsp::buf::StdBuf<complex_t> *)inputs[0]->write_buf())->max);
        ndsp::Block::start();
    }

    void FreqShift::set_params(nlohmann::json p)
    {
        if (p.contains("samplerate"))
            d_samplerate = p["samplerate"];
        if (p.contains("shift"))
            d_shift = p["shift"];

        set_freq(d_samplerate, d_shift);
    }

    void FreqShift::set_freq(double samplerate, double shift)
    {
        mtx.lock();
        phase = complex_t(1, 0);
        phase_delta = complex_t(cos(dsp::hz_to_rad(shift, samplerate)), sin(dsp::hz_to_rad(shift, samplerate)));
        mtx.unlock();
    }

    void FreqShift::set_freq_raw(double freq)
    {
        mtx.lock();
        phase_delta = complex_t(cosf(freq), sinf(freq));
        mtx.unlock();
    }

    void FreqShift::work()
    {
        if (!inputs[0]->read())
        {
            auto *rbuf = (ndsp::buf::StdBuf<complex_t> *)inputs[0]->read_buf();
            auto *wbuf = (ndsp::buf::StdBuf<complex_t> *)outputs[0]->write_buf();

            mtx.lock();
#if VOLK_VERSION >= 030100
            volk_32fc_s32fc_x2_rotator2_32fc((lv_32fc_t *)wbuf->dat, (lv_32fc_t *)rbuf->dat, (lv_32fc_t *)&phase_delta, (lv_32fc_t *)&phase, rbuf->cnt);
#else
            volk_32fc_s32fc_x2_rotator_32fc((lv_32fc_t *)wbuf->dat, (lv_32fc_t *)rbuf->dat, phase_delta, (lv_32fc_t *)&phase, rbuf->cnt);
#endif
            mtx.unlock();

            wbuf->cnt = rbuf->cnt;
            outputs[0]->write();
            inputs[0]->flush();
        }
    }
}

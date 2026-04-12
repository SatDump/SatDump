#include "ddc.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        DDC_Block::DDC_Block() : Block("ddc_c", {{"in", getTypeSampleType<complex_t>()}}, {/*{"out", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}*/}) {}

        DDC_Block::~DDC_Block() {}

        bool DDC_Block::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                if (iblk.terminatorShouldPropagate())
                {
                    vfos_mtx.lock();
                    for (auto &o : outputs)
                    {
                        IOInfo *i = ((IOInfo *)o.blkdata.get());
                        if (i->forward_terminator)
                            o.fifo->wait_enqueue(o.fifo->newBufferTerminator());
                    }
                    vfos_mtx.unlock();
                }
                inputs[0].fifo->free(iblk);
                return true;
            }

            vfos_mtx.lock();
            for (auto &o : outputs)
            {
                IOInfo *i = ((IOInfo *)o.blkdata.get());

                if (i->bandwidth == 0)
                {
                    DSPBuffer oblk = o.fifo->newBufferSamples(iblk.max_size, sizeof(complex_t));

                    if (i->frequency == 0)
                    {
                        memcpy(oblk.getSamples<complex_t>(), iblk.getSamples<complex_t>(), iblk.size * sizeof(complex_t));
                    }
                    else
                    {
#if VOLK_VERSION >= 030100
                        volk_32fc_s32fc_x2_rotator2_32fc((lv_32fc_t *)oblk.getSamples<complex_t>(), (const lv_32fc_t *)iblk.getSamples<complex_t>(), (lv_32fc_t *)&i->phase_delta,
                                                         (lv_32fc_t *)&i->phase, iblk.size);
#else
                        volk_32fc_s32fc_x2_rotator_32fc((lv_32fc_t *)oblk.getSamples<complex_t>(), (const lv_32fc_t *)iblk.getSamples<complex_t>(), i->phase_delta, (lv_32fc_t *)&i->phase, iblk.size);
#endif
                    }

                    if (i->decimation > 1)
                    {
                        int size_out = 0;
                        complex_t *ibuf = iblk.getSamples<complex_t>();
                        complex_t *obuf = oblk.getSamples<complex_t>();

                        int off = i->decimation - i->sample_index;
                        if (off == i->decimation)
                            off = 0;
                        i->sample_index += off;
                        int is;
                        for (is = off; is < iblk.size; is += i->decimation)
                        {
                            obuf[size_out++] = ibuf[is]; // * (i->phase = i->phase * i->phase_delta);
                            i->sample_index += i->decimation;
                            // printf(" %d,%d ", i, nsamples);
                        }
                        // printf(" end  \n");

                        i->sample_index -= is - iblk.size;
                        i->sample_index = i->sample_index % i->decimation;

                        oblk.size = size_out;
                    }
                    else
                    {
                        oblk.size = iblk.size;
                    }

                    o.fifo->wait_enqueue(oblk);
                }
                else
                {
                    DSPBuffer oblk = o.fifo->newBufferSamples(iblk.max_size, sizeof(complex_t));

                    if (i->frequency == 0)
                    {
                        memcpy(oblk.getSamples<complex_t>(), iblk.getSamples<complex_t>(), iblk.size * sizeof(complex_t));
                    }
                    else
                    {
#if VOLK_VERSION >= 030100
                        volk_32fc_s32fc_x2_rotator2_32fc((lv_32fc_t *)oblk.getSamples<complex_t>(), (const lv_32fc_t *)iblk.getSamples<complex_t>(), (lv_32fc_t *)&i->phase_delta,
                                                         (lv_32fc_t *)&i->phase, iblk.size);
#else
                        volk_32fc_s32fc_x2_rotator_32fc((lv_32fc_t *)oblk.getSamples<complex_t>(), (const lv_32fc_t *)iblk.getSamples<complex_t>(), i->phase_delta, (lv_32fc_t *)&i->phase, iblk.size);
#endif
                    }

                    oblk.size = i->filter.process(oblk.getSamples<complex_t>(), iblk.size, oblk.getSamples<complex_t>());

                    if (i->decimation > 1)
                    {
                        int size_out = 0;
                        complex_t *ibuf = oblk.getSamples<complex_t>();
                        complex_t *obuf = oblk.getSamples<complex_t>();

                        int off = i->decimation - i->sample_index;
                        if (off == i->decimation)
                            off = 0;
                        i->sample_index += off;
                        int is;
                        for (is = off; is < iblk.size; is += i->decimation)
                        {
                            obuf[size_out++] = ibuf[is]; // * (i->phase = i->phase * i->phase_delta);
                            i->sample_index += i->decimation;
                            // printf(" %d,%d ", i, nsamples);
                        }
                        // printf(" end  \n");

                        i->sample_index -= is - iblk.size;
                        i->sample_index = i->sample_index % i->decimation;

                        oblk.size = size_out;
                    }
                    else
                    {
                        oblk.size = iblk.size;
                    }

                    o.fifo->wait_enqueue(oblk);
                }
            }
            vfos_mtx.unlock();

            inputs[0].fifo->free(iblk);

            return false;
        }
    } // namespace ndsp
} // namespace satdump
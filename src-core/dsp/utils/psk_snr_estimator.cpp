#include "psk_snr_estimator.h"
#include "common/dsp/buffer.h"
#include "common/dsp/io/baseband_type.h"
#include "core/config.h"
#include "core/exception.h"
#include <complex.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <volk/volk_malloc.h>

namespace satdump
{
    namespace ndsp
    {
        PSKSnrEstimatorBlock::PSKSnrEstimatorBlock() : Block("psk_snr_estimator_c", {{"in", DSP_SAMPLE_TYPE_CF32}}, {}) {}

        PSKSnrEstimatorBlock::~PSKSnrEstimatorBlock() {}

        bool PSKSnrEstimatorBlock::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                inputs[0].fifo->free(iblk);
                return true;
            }

            int nsam = iblk.size;
            complex_t *ibuf = iblk.getSamples<complex_t>();

            estimator.update(ibuf, nsam);

            inputs[0].fifo->free(iblk);

            return false;
        }
    } // namespace ndsp
} // namespace satdump
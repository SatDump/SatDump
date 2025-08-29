#include "cyclostationary_analysis.h"

namespace satdump
{
    namespace ndsp
    {
        CyclostationaryAnalysis::CyclostationaryAnalysis() : Block("cyclostationary_analysis_cf", {{"in", DSP_SAMPLE_TYPE_CF32}}, {{"out", DSP_SAMPLE_TYPE_F32}}) {}

        CyclostationaryAnalysis::~CyclostationaryAnalysis() {}

        bool CyclostationaryAnalysis::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                if (iblk.terminatorShouldPropagate())
                    outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
                inputs[0].fifo->free(iblk);
                return true;
            }

            auto oblk = outputs[0].fifo->newBufferSamples<float>(iblk.max_size * 2); // complex_t = 2 floats
            complex_t *ibuf = iblk.getSamples<complex_t>();
            complex_t *obuf = oblk.getSamples<complex_t>();
            float *obuff = oblk.getSamples<float>();

            volk_32fc_conjugate_32fc((lv_32fc_t *)obuf, (const lv_32fc_t *)ibuf, iblk.size);
            volk_32fc_x2_multiply_32fc((lv_32fc_t *)obuf, (const lv_32fc_t *)obuf, (const lv_32fc_t *)ibuf, iblk.size);
            volk_32fc_magnitude_32f(obuff, (const lv_32fc_t *)obuf, iblk.size);

            oblk.size = iblk.size;
            outputs[0].fifo->wait_enqueue(oblk);
            inputs[0].fifo->free(iblk);

            return false;
        }
    } // namespace ndsp
} // namespace satdump

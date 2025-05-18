#include "cyclostationary_analysis.h"

namespace satdump
{
    namespace ndsp
    {
        CyclostationaryAnalysis::CyclostationaryAnalysis()
            : Block("cyclostationary_analysis_cc", {{"in", DSP_SAMPLE_TYPE_CF32}}, {{"out", DSP_SAMPLE_TYPE_CF32}}) // TODOREWORK change "out" back to F32
        {
        }

        CyclostationaryAnalysis::~CyclostationaryAnalysis() {}

        bool CyclostationaryAnalysis::work()
        {
            DSPBuffer iblk;
            inputs[0].fifo->wait_dequeue(iblk);

            if (iblk.isTerminator())
            {
                if (iblk.terminatorShouldPropagate())
                    outputs[0].fifo->wait_enqueue(DSPBuffer::newBufferTerminator());
                iblk.free();
                return true;
            }

            auto oblk = DSPBuffer::newBufferSamples<float>(iblk.max_size * 2); // complex_t = 2 floats
            complex_t *ibuf = iblk.getSamples<complex_t>();
            complex_t *obuf = oblk.getSamples<complex_t>();
            // float *obuff = oblk.getSamples<float>(); // TODOREWORK uncomment to use float output again

            volk_32fc_conjugate_32fc((lv_32fc_t *)obuf, (const lv_32fc_t *)ibuf, iblk.size);
            volk_32fc_x2_multiply_32fc((lv_32fc_t *)obuf, (const lv_32fc_t *)obuf, (const lv_32fc_t *)ibuf, iblk.size);
            // volk_32fc_magnitude_32f(obuff, (const lv_32fc_t *)obuf, iblk.size);  // TODOREWORK fix the output and make it the magnitude again

            oblk.size = iblk.size;
            outputs[0].fifo->wait_enqueue(oblk);
            iblk.free();

            return false;
        }
    } // namespace ndsp
} // namespace satdump

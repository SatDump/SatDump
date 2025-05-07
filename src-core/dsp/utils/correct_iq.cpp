#include "correct_iq.h"
#include "common/dsp/complex.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        CorrectIQBlock<T>::CorrectIQBlock()
            : BlockSimple<T, T>(std::is_same_v<T, complex_t> ? "correct_iq_cc" : "correct_iq_ff", {{"in", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}},
                                {{"out", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}})
        {
            beta = 1.0f - alpha;
        }

        template <typename T>
        CorrectIQBlock<T>::~CorrectIQBlock()
        {
        }

        template <typename T>
        uint32_t CorrectIQBlock<T>::process(T *in, uint32_t nsamples, T *out)
        {
            for (uint32_t i = 0; i < nsamples; i++)
            {
                acc = acc * beta + in[i] * alpha;
                out[i] = in[i] - acc;
            }
            return nsamples;
        }
        template class CorrectIQBlock<complex_t>;
        template class CorrectIQBlock<float>;
    } // namespace ndsp
} // namespace satdump

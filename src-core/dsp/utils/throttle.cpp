#include "throttle.h"
#include "common/dsp/complex.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        ThrottleBlock<T>::ThrottleBlock()
            : BlockSimple<T, T>(std::is_same_v<T, complex_t> ? "throttle_cc" : "throttle_ff", //
                                {{"in", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}},
                                {{"out", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}})
        {
        }

        template <typename T>
        ThrottleBlock<T>::~ThrottleBlock()
        {
        }

        template <typename T>
        uint32_t ThrottleBlock<T>::process(T *in, uint32_t nsamples, T *out)
        {
            // Copy over samples
            memcpy(out, in, nsamples * sizeof(T));

            if (needs_reinit)
            {
                sample_time_period = std::chrono::duration<double>(1.0 / (double)samplerate);
                start_time_point = std::chrono::steady_clock::now();
                total_samples = 0;

                needs_reinit = false;
            }

            total_samples += nsamples;
            auto now = std::chrono::steady_clock::now();
            auto expected_time = start_time_point + sample_time_period * total_samples;

            if (expected_time < now)
            {
                // We got behind, either because we're slow or fast
                // mode stopped. Reset counters and carry on.
                start_time_point = now;
                total_samples = 0;
            }
            else
                std::this_thread::sleep_until(expected_time);

            return nsamples;
        }
        template class ThrottleBlock<complex_t>;
        template class ThrottleBlock<float>;
    } // namespace ndsp
} // namespace satdump

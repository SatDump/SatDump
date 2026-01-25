#include "cosine_source.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        CosBlock<T>::CosBlock()
            : Block("cosine_source_" + getShortTypeName<T>(), {}, //
                    {{"out", getTypeSampleType<T>()}})
        {
            d_buffer_size = p_buffer_size;
        }

        template <typename T>
        void CosBlock<T>::init()
        {
            d_samprate = p_samprate;
            d_freq = p_freq;
            d_amp = p_amp;
            d_phase = p_phase;
        }

        template <typename T>
        CosBlock<T>::~CosBlock()
        {
        }

        template <typename T>
        bool CosBlock<T>::work()
        {
            if (!work_should_exit)
            {
                auto oblk = outputs[0].fifo->newBufferSamples(d_buffer_size, sizeof(T));
                T *obuf = oblk.template getSamples<T>();

                if (needs_reinit)
                {
                    needs_reinit = false;
                    init();
                }

                int nsamples = d_buffer_size;

                double noise_imp = 1;
                double noise_snr_linear = 1e3 * pow(10, -150 / 10);
                // T noise_target_level = T(sqrt((noise_imp * noise_snr_linear) / 2));

                for (size_t i = 0; i < nsamples; i++)
                {
                    obuf[i] = tmp_val;
                    T t = i / (double)d_samprate;

                    d_phase += ((d_phase >= 2.0 * M_PI) * -2.0 * M_PI) + ((d_phase < 0.0) * 2.0 * M_PI);

                    T cosWave = d_amp * T(cos(2.0 * M_PI * d_freq * t + d_phase));

                    obuf[i] = cosWave;

                    T ns = T(sqrt((noise_imp * noise_snr_linear) / 2)) * T(d_rng.gasdev());
                    obuf[i] = obuf[i] + ns;
                }

                oblk.size = d_buffer_size;
                outputs[0].fifo->wait_enqueue(oblk);
            }
            else
            {
                outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
                return true;
            }

            return false;
        }
        // template class CosBlock<complex_t>;
        template class CosBlock<float>;
    } // namespace ndsp
} // namespace satdump

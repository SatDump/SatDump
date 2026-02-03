#include "waveform.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        WaveformBlock<T>::WaveformBlock()
            : Block("waveform_" + getShortTypeName<T>(), {}, //
                    {{"out", getTypeSampleType<T>()}})
        {
            d_buffer_size = p_buffer_size;
        }

        template <typename T>
        void WaveformBlock<T>::init()
        {
            d_waveform = p_waveform;
            d_samprate = p_samprate;
            d_freq = p_freq;
            d_amp = p_amp;
            d_phase = p_phase;
        }

        template <typename T>
        WaveformBlock<T>::~WaveformBlock()
        {
        }

        template <typename T>
        bool WaveformBlock<T>::work()
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

                if (p_waveform == "cosine")
                {
                    for (size_t i = 0; i < nsamples; i++)
                    {
                        float t = i / (double)d_samprate;

                        d_phase += ((d_phase >= 2.0 * M_PI) * -2.0 * M_PI) + ((d_phase < 0.0) * 2.0 * M_PI);

                        float cosWave = d_amp * float(cos(2.0 * M_PI * d_freq * t + d_phase));

                        tmp_val_f = cosWave;

                        float ns = float(sqrt((noise_imp * noise_snr_linear) / 2)) * float(d_rng.gasdev());

                        tmp_val_f += ns;

                        if constexpr (std::is_same_v<T, float>)
                        {
                            obuf[i] = tmp_val_f;
                        }

                        if constexpr (std::is_same_v<T, complex_t>)
                        {
                            float sinOfCos = d_amp * float(sin(2.0 * M_PI * d_freq * t + d_phase));

                            sinOfCos += ns;

                            obuf[i] = complex_t(tmp_val_f, sinOfCos);
                        }
                    }
                }
                if (p_waveform == "sine")
                {

                    for (size_t i = 0; i < nsamples; i++)
                    {
                        float t = i / (double)d_samprate;

                        d_phase += ((d_phase >= 2.0 * M_PI) * -2.0 * M_PI) + ((d_phase < 0.0) * 2.0 * M_PI);

                        float sinWave = d_amp * float(sin(2.0 * M_PI * d_freq * t + d_phase));

                        tmp_val_f = sinWave;

                        float ns = float(sqrt((noise_imp * noise_snr_linear) / 2)) * float(d_rng.gasdev());

                        tmp_val_f += ns;

                        if constexpr (std::is_same_v<T, float>)
                        {
                            obuf[i] = tmp_val_f;
                        }

                        if constexpr (std::is_same_v<T, complex_t>)
                        {
                            float cosOfSin = d_amp * float(cos(2.0 * M_PI * d_freq * t + d_phase));

                            cosOfSin += ns;

                            obuf[i] = complex_t(cosOfSin, tmp_val_f);
                        }
                    }
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
        template class WaveformBlock<complex_t>;
        template class WaveformBlock<float>;
    } // namespace ndsp
} // namespace satdump

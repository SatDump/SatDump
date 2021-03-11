#include "fir_gen.h"
#include <cmath>
#include <stdexcept>

namespace dsp
{
    namespace firgen
    {
        int compute_ntaps(double sampling_freq, double transition_width, fft::window::win_type window_type, double beta)
        {
            double a = fft::window::max_attenuation(
                static_cast<fft::window::win_type>(window_type), beta);
            int ntaps = (int)(a * sampling_freq / (22.0 * transition_width));
            if ((ntaps & 1) == 0) // if even...
                ntaps++;          // ...make odd

            return ntaps;
        }

        void sanity_check_1f(double sampling_freq,
                             double fa, // cutoff freq
                             double transition_width)
        {
            if (sampling_freq <= 0.0)
                throw std::out_of_range("firdes check failed: sampling_freq > 0");

            if (fa <= 0.0 || fa > sampling_freq / 2)
                throw std::out_of_range("firdes check failed: 0 < fa <= sampling_freq / 2");

            if (transition_width <= 0)
                throw std::out_of_range("firdes check failed: transition_width > 0");
        }

        std::vector<float> window(fft::window::win_type type, int ntaps, double beta)
        {
            return fft::window::build(type, ntaps, beta);
        }

        std::vector<float>
        low_pass(double gain,
                 double sampling_freq,
                 double cutoff_freq,      // Hz center of transition band
                 double transition_width, // Hz width of transition band
                 fft::window::win_type window_type,
                 double beta) // used only with Kaiser
        {
            sanity_check_1f(sampling_freq, cutoff_freq, transition_width);

            int ntaps = compute_ntaps(sampling_freq, transition_width, window_type, beta);

            // construct the truncated ideal impulse response
            // [sin(x)/x for the low pass case]

            std::vector<float> taps(ntaps);
            std::vector<float> w = window(window_type, ntaps, beta);

            int M = (ntaps - 1) / 2;
            double fwT0 = 2 * M_PI * cutoff_freq / sampling_freq;

            for (int n = -M; n <= M; n++)
            {
                if (n == 0)
                    taps[n + M] = fwT0 / M_PI * w[n + M];
                else
                {
                    // a little algebra gets this into the more familiar sin(x)/x form
                    taps[n + M] = sin(n * fwT0) / (n * M_PI) * w[n + M];
                }
            }

            // find the factor to normalize the gain, fmax.
            // For low-pass, gain @ zero freq = 1.0

            double fmax = taps[0 + M];
            for (int n = 1; n <= M; n++)
                fmax += 2 * taps[n + M];

            gain /= fmax; // normalize

            for (int i = 0; i < ntaps; i++)
                taps[i] *= gain;

            return taps;
        }

        std::vector<float> root_raised_cosine(double gain, double sampling_freq, double symbol_rate, double alpha, int ntaps)
        {
            ntaps |= 1; // ensure that ntaps is odd

            double spb = sampling_freq / symbol_rate; // samples per bit/symbol
            std::vector<float> taps(ntaps);
            double scale = 0;
            for (int i = 0; i < ntaps; i++)
            {
                double x1, x2, x3, num, den;
                double xindx = i - ntaps / 2;
                x1 = M_PI * xindx / spb;
                x2 = 4 * alpha * xindx / spb;
                x3 = x2 * x2 - 1;

                if (fabs(x3) >= 0.000001)
                { // Avoid Rounding errors...
                    if (i != ntaps / 2)
                        num = cos((1 + alpha) * x1) +
                              sin((1 - alpha) * x1) / (4 * alpha * xindx / spb);
                    else
                        num = cos((1 + alpha) * x1) + (1 - alpha) * M_PI / (4 * alpha);
                    den = x3 * M_PI;
                }
                else
                {
                    if (alpha == 1)
                    {
                        taps[i] = -1;
                        scale += taps[i];
                        continue;
                    }
                    x3 = (1 - alpha) * x1;
                    x2 = (1 + alpha) * x1;
                    num = (sin(x2) * (1 + alpha) * M_PI -
                           cos(x3) * ((1 - alpha) * M_PI * spb) / (4 * alpha * xindx) +
                           sin(x3) * spb * spb / (4 * alpha * xindx * xindx));
                    den = -32 * M_PI * alpha * alpha * xindx / spb;
                }
                taps[i] = 4 * alpha * num / den;
                scale += taps[i];
            }

            for (int i = 0; i < ntaps; i++)
                taps[i] = taps[i] * gain / scale;

            return taps;
        }

        //
        // Hilbert Transform
        //

        std::vector<float> hilbert(unsigned int ntaps, fft::window::win_type windowtype, double beta)
        {
            if (!(ntaps & 1))
                throw std::out_of_range("Hilbert:  Must have odd number of taps");

            std::vector<float> taps(ntaps);
            std::vector<float> w = window(windowtype, ntaps, beta);
            unsigned int h = (ntaps - 1) / 2;
            float gain = 0;
            for (unsigned int i = 1; i <= h; i++)
            {
                if (i & 1)
                {
                    float x = 1 / (float)i;
                    taps[h + i] = x * w[h + i];
                    taps[h - i] = -x * w[h - i];
                    gain = taps[h + i] - gain;
                }
                else
                    taps[h + i] = taps[h - i] = 0;
            }

            gain = 2 * fabs(gain);
            for (unsigned int i = 0; i < ntaps; i++)
                taps[i] /= gain;
            return taps;
        }

        std::vector<float> design_resampler_filter_float(const unsigned interpolation, const unsigned decimation, const float fractional_bw)
        {

            if (fractional_bw >= 0.5 || fractional_bw <= 0)
            {
                throw std::range_error("Invalid fractional_bandwidth, must be in (0, 0.5)");
            }

            // These are default values used to generate the filter when no taps are known
            //  Pulled from rational_resampler.py
            float beta = 7.0;
            float halfband = 0.5;
            float rate = float(interpolation) / float(decimation);
            float trans_width, mid_transition_band;

            if (rate >= 1.0)
            {
                trans_width = halfband - fractional_bw;
                mid_transition_band = halfband - trans_width / 2.0;
            }
            else
            {
                trans_width = rate * (halfband - fractional_bw);
                mid_transition_band = rate * halfband - trans_width / 2.0;
            }

            return low_pass(interpolation,       /* gain */
                            interpolation,       /* Fs */
                            mid_transition_band, /* trans mid point */
                            trans_width,         /* transition width */
                            fft::window::WIN_KAISER,
                            beta); /* beta*/
        }
    } // namespace firgen
} // namespace libdsp

#include "firdes.h"
#include <algorithm>
#include <numeric>

#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif

namespace dsp
{
    namespace firdes
    {
        std::vector<float> convolve(std::vector<float> u, std::vector<float> v)
        {
            std::vector<float> w;
            int su = u.size(), sv = v.size();
            int size = su + sv - 1;
            double sum = 0;
            for (int i = 0; i < size; i++)
            {
                int iter = i;
                for (int j = 0; j <= i; j++)
                {
                    if (!((int)u.size() <= j || (int)v.size() <= iter))
                        sum += u.at(j) * v.at(iter);
                    iter--;
                }
                w.push_back(sum);
                sum = 0;
            }
            return w;
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

        std::vector<float> low_pass(double gain, double sampling_freq, double cutoff_freq, double transition_width, fft::window::win_type window_type, double beta) // used only with Kaiser
        {
            double a = fft::window::max_attenuation(static_cast<fft::window::win_type>(window_type), beta);
            int ntaps = (int)(a * sampling_freq / (22.0 * transition_width));
            if ((ntaps & 1) == 0) // if even...
                ntaps++;          // ...make odd

            // construct the truncated ideal impulse response
            // [sin(x)/x for the low pass case]

            std::vector<float> taps(ntaps);
            std::vector<float> w = fft::window::build(window_type, ntaps, beta);

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

        std::vector<float> design_resampler_filter_float(const unsigned interpolation, const unsigned decimation, const float fractional_bw)
        {
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

        std::vector<float> gaussian(double gain, double spb, double bt, int ntaps)
        {
            std::vector<float> taps(ntaps);
            double scale = 0;
            double dt = 1.0 / spb;
            double s = 1.0 / (sqrt(log(2.0)) / (2 * M_PI * bt));
            double t0 = -0.5 * ntaps;
            double ts;
            for (int i = 0; i < ntaps; i++)
            {
                t0++;
                ts = s * dt * t0;
                taps[i] = exp(-0.5 * ts * ts);
                scale += taps[i];
            }
            for (int i = 0; i < ntaps; i++)
                taps[i] = taps[i] / scale * gain;

            return taps;
        }
    };

    namespace fft
    {
#define IzeroEPSILON 1E-21 /* Max error acceptable in Izero */

        double Izero(double x)
        {
            double sum, u, halfx, temp;
            int n;

            sum = u = n = 1;
            halfx = x / 2.0;
            do
            {
                temp = halfx / (double)n;
                n += 1;
                temp *= temp;
                u *= temp;
                sum += u;
            } while (u >= IzeroEPSILON * sum);
            return (sum);
        }

        std::vector<float> window::coswindow(int ntaps, float c0, float c1, float c2)
        {
            std::vector<float> taps(ntaps);
            float M = static_cast<float>(ntaps - 1);

            for (int n = 0; n < ntaps; n++)
                taps[n] = c0 - c1 * cosf((2.0f * M_PI * n) / M) +
                          c2 * cosf((4.0f * M_PI * n) / M);
            return taps;
        }

        std::vector<float> window::coswindow(int ntaps, float c0, float c1, float c2, float c3)
        {
            std::vector<float> taps(ntaps);
            float M = static_cast<float>(ntaps - 1);

            for (int n = 0; n < ntaps; n++)
                taps[n] = c0 - c1 * cosf((2.0f * M_PI * n) / M) +
                          c2 * cosf((4.0f * M_PI * n) / M) -
                          c3 * cosf((6.0f * M_PI * n) / M);
            return taps;
        }

        std::vector<float> window::coswindow(int ntaps, float c0, float c1, float c2, float c3, float c4)
        {
            std::vector<float> taps(ntaps);
            float M = static_cast<float>(ntaps - 1);

            for (int n = 0; n < ntaps; n++)
                taps[n] = c0 - c1 * cosf((2.0f * M_PI * n) / M) +
                          c2 * cosf((4.0f * M_PI * n) / M) -
                          c3 * cosf((6.0f * M_PI * n) / M) +
                          c4 * cosf((8.0f * M_PI * n) / M);
            return taps;
        }

        std::vector<float> window::rectangular(int ntaps)
        {
            std::vector<float> taps(ntaps);
            for (int n = 0; n < ntaps; n++)
                taps[n] = 1;
            return taps;
        }

        std::vector<float> window::hamming(int ntaps)
        {
            std::vector<float> taps(ntaps);
            float M = static_cast<float>(ntaps - 1);

            for (int n = 0; n < ntaps; n++)
                taps[n] = 0.54 - 0.46 * cos((2 * M_PI * n) / M);
            return taps;
        }

        std::vector<float> window::hann(int ntaps)
        {
            std::vector<float> taps(ntaps);
            float M = static_cast<float>(ntaps - 1);

            for (int n = 0; n < ntaps; n++)
                taps[n] = 0.5 - 0.5 * cos((2 * M_PI * n) / M);
            return taps;
        }

        std::vector<float> window::blackman(int ntaps)
        {
            return coswindow(ntaps, 0.42, 0.5, 0.08);
        }

        std::vector<float> window::blackman_harris(int ntaps, int atten)
        {
            switch (atten)
            {
            case (61):
                return coswindow(ntaps, 0.42323, 0.49755, 0.07922);
            case (67):
                return coswindow(ntaps, 0.44959, 0.49364, 0.05677);
            case (74):
                return coswindow(ntaps, 0.40271, 0.49703, 0.09392, 0.00183);
            case (92):
                return coswindow(ntaps, 0.35875, 0.48829, 0.14128, 0.01168);
            default:
                throw std::out_of_range("window::blackman_harris: unknown attenuation value "
                                        "(must be 61, 67, 74, or 92)");
            }
        }

        std::vector<float> window::kaiser(int ntaps, double beta)
        {
            if (beta < 0)
                throw std::out_of_range("window::kaiser: beta must be >= 0");

            std::vector<float> taps(ntaps);

            double IBeta = 1.0 / Izero(beta);
            double inm1 = 1.0 / ((double)(ntaps - 1));
            double temp;

            /* extracting first and last element out of the loop, since
            sqrt(1.0-temp*temp) might trigger unexpected floating point behaviour
            if |temp| = 1.0+epsilon, which can happen for i==0 and
            1/i==1/(ntaps-1)==inm1 ; compare
            https://github.com/gnuradio/gnuradio/issues/1348 .
            In any case, the 0. Bessel function of first kind is 1 at point 0.
            */
            taps[0] = IBeta;
            for (int i = 1; i < ntaps - 1; i++)
            {
                temp = 2 * i * inm1 - 1;
                taps[i] = Izero(beta * sqrt(1.0 - temp * temp)) * IBeta;
            }
            taps[ntaps - 1] = IBeta;
            return taps;
        }

        std::vector<float> window::bartlett(int ntaps)
        {
            std::vector<float> taps(ntaps);
            float M = static_cast<float>(ntaps - 1);

            for (int n = 0; n < ntaps / 2; n++)
                taps[n] = 2 * n / M;
            for (int n = ntaps / 2; n < ntaps; n++)
                taps[n] = 2 - 2 * n / M;

            return taps;
        }

        std::vector<float> window::flattop(int ntaps)
        {
            double scale = 4.63867;
            return coswindow(ntaps, 1.0 / scale, 1.93 / scale, 1.29 / scale, 0.388 / scale, 0.028 / scale);
        }

        double window::max_attenuation(win_type type, double beta)
        {
            switch (type)
            {
            case (WIN_HAMMING):
                return 53;
                break;
            case (WIN_HANN):
                return 44;
                break;
            case (WIN_BLACKMAN):
                return 74;
                break;
            case (WIN_RECTANGULAR):
                return 21;
                break;
            case (WIN_KAISER):
                return (beta / 0.1102 + 8.7);
                break;
            case (WIN_BLACKMAN_hARRIS):
                return 92;
                break;
            case (WIN_BARTLETT):
                return 27;
                break;
            case (WIN_FLATTOP):
                return 93;
                break;
            default:
                throw std::out_of_range("window::max_attenuation: unknown window type provided.");
            }
        }

        std::vector<float> window::build(win_type type, int ntaps, double beta, const bool normalize)
        {
            // If we want a normalized window, we get a non-normalized one first, then
            // normalize it here:
            if (normalize)
            {
                auto win = build(type, ntaps, beta, false);
                const double pwr_acc = std::accumulate(win.cbegin(),
                                                       win.cend(),
                                                       0.0,
                                                       [](const double a, const double b)
                                                       { return a + b * b; }) /
                                       win.size();
                const float norm_fac = static_cast<float>(std::sqrt(pwr_acc));
                std::transform(win.begin(), win.end(), win.begin(), [norm_fac](const float tap)
                               { return tap / norm_fac; });
                return win;
            }

            // Create non-normalized window:
            switch (type)
            {
            case WIN_RECTANGULAR:
                return rectangular(ntaps);
            case WIN_HAMMING:
                return hamming(ntaps);
            case WIN_HANN:
                return hann(ntaps);
            case WIN_BLACKMAN:
                return blackman(ntaps);
            case WIN_BLACKMAN_hARRIS:
                return blackman_harris(ntaps);
            case WIN_KAISER:
                return kaiser(ntaps, beta);
            case WIN_BARTLETT:
                return bartlett(ntaps);
            case WIN_FLATTOP:
                return flattop(ntaps);
            default:
                throw std::out_of_range("window::build: type out of range");
            }
        }
    };
};
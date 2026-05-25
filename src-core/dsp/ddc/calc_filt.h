#pragma once

#include "common/dsp/complex.h"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <vector>

namespace satdump
{
    namespace ndsp
    {
        namespace filt
        {
            inline double bessi0(double x)
            /*------------------------------------------------------------*/
            /* PURPOSE: Evaluate modified Bessel function In(x) and n=0.  */
            /*------------------------------------------------------------*/
            {
                double ax, ans;
                double y;

                if ((ax = fabs(x)) < 3.75)
                {
                    y = x / 3.75, y = y * y;
                    ans = 1.0 + y * (3.5156229 + y * (3.0899424 + y * (1.2067492 + y * (0.2659732 + y * (0.360768e-1 + y * 0.45813e-2)))));
                }
                else
                {
                    y = 3.75 / ax;
                    ans = (exp(ax) / sqrt(ax)) *
                          (0.39894228 +
                           y * (0.1328592e-1 + y * (0.225319e-2 + y * (-0.157565e-2 + y * (0.916281e-2 + y * (-0.2057706e-1 + y * (0.2635537e-1 + y * (-0.1647633e-1 + y * 0.392377e-2))))))));
                }
                return ans;
            }

            //%  Fs=Sampling frequency
            //%      * Fa=Low freq ideal cut off (0=low pass)
            //%      * Fb=High freq ideal cut off (Fs/2=high pass)
            //%      * Att=Minimum stop band attenuation (>21dB)
            inline std::vector<double> calc_filter(double Fs, double Fa, double Fb, int M, double Att)
            {
                std::vector<double> _H;

                int k, j;

                int len = 2 * M + 1;
                double Alpha, C, x, y;
                double *W;

                _H.resize(len);
                Alpha = .1102 * (Att - 8.7);

                // compute kaiser window of length 2M+1
                W = (double *)malloc(len * sizeof(double));
                assert(W != nullptr);
                C = bessi0(Alpha * M_PI);

                for (k = 0; k < len; k++)
                {
                    y = k * 1.0 / M - 1.0;
                    x = M_PI * Alpha * sqrt(1 - y * y);
                    W[k] = bessi0(x) / C;
                    // printf("h(%d)=%0.8f;\n", k+1, W[k]);
                }

                k = 0;
                for (j = -M; j <= M; j++)
                {
                    if (j == 0)
                    {
                        _H[k] = 2 * (Fb - Fa) / Fs;
                    }
                    else
                    {
                        _H[k] = 1 / (M_PI * j) * (sin(2 * M_PI * j * Fb / Fs) - sin(2 * M_PI * j * Fa / Fs)) * W[k];
                    }
                    k++;
                }

                //  *Ntaps = len;
                //    for( k=0 ; k < len ; k++ ) {
                //        printf("h(%d)=%0.8f;\n", k+1, H[k]*1000000);
                //    }
                free(W);
                return _H;
            }

            inline std::vector<complex_t> generateTaps(double Fs, double Fa, double Fb, int M, double Att, double fwT0)
            {
                std::vector<complex_t> ctaps;
                auto taps = calc_filter(Fs, Fa, Fb, M, Att);

                for (int i = 0; i < taps.size(); i++)
                {
                    complex_t v = float(taps[i]) * exp(std::complex<float>(0, i * fwT0));
                    ctaps.push_back(v);
                }

                return ctaps;
            }
        } // namespace filt
    } // namespace ndsp
} // namespace satdump
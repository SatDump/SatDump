#include "spectral_align.h"
#include "common/dsp/complex.h"
#include <cmath>
#include <cstdlib>
#include <fftw3.h>
#include <volk/volk.h>

namespace satdump
{
    namespace image
    {
        // Zero-mean unit-variance normalisation
        inline void normalize(float *v, size_t len)
        {
            double mean = 0;
            for (size_t i = 0; i < len; i++)
                mean += v[i];
            mean /= len;
            double var = 0;
            for (size_t i = 0; i < len; i++)
                var += (v[i] - mean) * (v[i] - mean);
            double sd = sqrt(var / double(len) + 1e-8f);
            for (size_t i = 0; i < len; i++)
                v[i] = (v[i] - mean) / sd;
        }

        // Simple Hann window (deal with image borders causing trouble)
        inline void applyHann(float *data, int w, int h)
        {
            for (int y = 0; y < h; y++)
            {
                float wy = 0.5f * (1.f - cos(2.f * M_PI * y / (h - 1)));
                for (int x = 0; x < w; x++)
                {
                    float wx = 0.5f * (1.f - cos(2.f * M_PI * x / (w - 1)));
                    data[y * w + x] *= wx * wy;
                }
            }
        }

        // Fit a parabole on 3 provided points
        inline double parabolaEstimate(float vm, float v0, float vp)
        {
            float denom = 2.f * (vm - 2.f * v0 + vp);
            if (std::abs(denom) == 0)
                return 0;
            return (vm - vp) / denom;
        };

        bool correlate_fft2d(image::Image &ref, image::Image &tgt, double &out_x, double &out_y)
        {
            if (ref.depth() != tgt.depth())
                return true;
            if (ref.width() != tgt.width())
                return true;
            if (ref.height() != tgt.height())
                return true;
            if (ref.channels() != 1)
                return true;
            if (tgt.channels() != 1)
                return true;

            int W = ref.width();
            int H = ref.height();
            int nc = W / 2 + 1; // complex columns after FFT
            int N = H * W;

            // Buffers
            float *r_in = fftwf_alloc_real(N);
            float *t_in = fftwf_alloc_real(N);
            fftwf_complex *r_out = fftwf_alloc_complex(H * nc);
            fftwf_complex *t_out = fftwf_alloc_complex(H * nc);
            fftwf_complex *cps = fftwf_alloc_complex(H * nc); // cross-power spectrum
            float *corr = fftwf_alloc_real(N);

            // Plans
            fftwf_plan p_r = fftwf_plan_dft_r2c_2d(H, W, r_in, r_out, FFTW_ESTIMATE);
            fftwf_plan p_t = fftwf_plan_dft_r2c_2d(H, W, t_in, t_out, FFTW_ESTIMATE);
            fftwf_plan p_inv = fftwf_plan_dft_c2r_2d(H, W, cps, corr, FFTW_ESTIMATE);

            // Normalize, 2D Hann Window on both
            for (int i = 0; i < ref.size(); i++)
                r_in[i] = ref.getf(i);
            normalize(r_in, W * H);
            applyHann(r_in, W, H);

            for (int i = 0; i < tgt.size(); i++)
                t_in[i] = tgt.getf(i);
            normalize(t_in, W * H);
            applyHann(t_in, W, H);

            // Perform FFTs
            fftwf_execute(p_r);
            fftwf_execute(p_t);

            // R * conj(T) / abs(R * conj(T))
            for (int i = 0; i < H * nc; i++)
            {
                complex_t *cr_out = (complex_t *)r_out;
                complex_t *ct_out = (complex_t *)t_out;

                complex_t r_conj_t = cr_out[i] * ct_out[i].conj();
                float m = sqrt(r_conj_t.real * r_conj_t.real + r_conj_t.imag * r_conj_t.imag);
                if (m > 0) // Prevent div by zero
                    ((complex_t *)cps)[i] = (cr_out[i] * ct_out[i].conj()) / m;
                else
                    ((complex_t *)cps)[i] = 0;
            }

            // Perform IFFT
            fftwf_execute(p_inv);

            // Normalize (Divide by N, since IFFT)
            for (int i = 0; i < N; i++)
                corr[i] *= (1.f / N);

            // Find peak
            uint32_t peakIdx;
            volk_32f_index_max_32u(&peakIdx, corr, N);
            int py = peakIdx / W;
            int px = peakIdx % W;

            // Fit a parabola to gain more precision (otherwise it wasn't that great on some instruments)
            float vm_x = corr[py * W + (px - 1 + W) % W];
            float vp_x = corr[py * W + (px + 1) % W];
            float vm_y = corr[((py - 1 + H) % H) * W + px];
            float vp_y = corr[((py + 1) % H) * W + px];

            out_x = px + parabolaEstimate(vm_x, corr[py * W + px], vp_x);
            out_y = py + parabolaEstimate(vm_y, corr[py * W + px], vp_y);

            // Remove offset
            if (out_x > W / 2.0)
                out_x -= W;
            if (out_y > H / 2.0)
                out_y -= H;

            // Free everything
            fftwf_destroy_plan(p_r);
            fftwf_destroy_plan(p_t);
            fftwf_destroy_plan(p_inv);
            fftwf_free(r_in);
            fftwf_free(t_in);
            fftwf_free(r_out);
            fftwf_free(t_out);
            fftwf_free(cps);
            fftwf_free(corr);

            return false;
        }
    } // namespace image
} // namespace satdump
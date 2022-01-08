#include "fft.h"
#include <fftw3.h>
#include <cmath>

namespace image
{
    /*
    I spent hours trying to make this work... And ended up taking a look at
    https://github.com/rpeyron/plugin-gimp-fourier/blob/main/fourier.c
    and porting it over.
    All credits go to the original authors.
   */

    float normalize(int x, int y, int width, int height)
    {
        float cx = (float)abs(x - width / 2);
        float cy = (float)abs(y - height / 2);
        float energy = (sqrt(cx) + sqrt(cy));
        return energy * energy;
    }

    int round_gint(float value)
    {
        float floored = floor(value);
        if (value - floored > 0.5)
        {
            return (int)(floored + 1);
        }
        return (int)floored;
    }

    int boost(float value)
    {
        float bounded = fabs(value / 40960.0);
        int boosted = round_gint(32768 * sqrt(bounded));
        boosted = (value > 0) ? boosted : -boosted;
        return boosted;
    }

    float unboost(float value)
    {
        float bounded = fabs(value / 32768.0);
        float unboosted = 40960.0 * bounded * bounded;
        unboosted = (value > 0) ? unboosted : -unboosted;
        return unboosted;
    }

    unsigned short get_scaled(int i)
    {
        return (unsigned short)(i >= (int)32768) ? 65535 : ((i <= (int)-32768) ? 0 : i + 32768);
    }

    float get_unscaled(unsigned short c)
    {
        return (float)(c)-32768.0;
    }

    int pixel_imag(int row, int col, int h, int w)
    {
        if ((row == 0 && h % 2 == 0) || row == h / 2)
            return col > w / 2;
        else
            return row > h / 2;
    }

    void map(int row, int col, int h, int w, int *row2, int *col2)
    {
        *row2 = (row + (h + 1) / 2) % h; /* shift origin */
        *col2 = (col + (w + 1) / 2) % w;
        if (*col2 > w / 2)
        { /* wrap */
            *row2 = (h - *row2) % h;
            *col2 = w - *col2;
        }
        *col2 *= 2; /* unit = real number */
        if (pixel_imag(row, col, h, w))
            (*col2)++; /* take imaginary part */
    }

    void fft_forward(Image<uint16_t> &image)
    {
        int w = image.width();
        int h = image.height();

        int pad = (w & 1) ? 1 : 2;

        float *fft_in = new float[h * w * 2];
        float *fft_out = new float[h * w * 2];

        for (int y = 0; y < h; y++)
        {
            for (int i = 0; i < w; i++)
            {
                fft_in[y * w + i] = image[y * w + i];
            }
        }

        fftwf_plan p = fftwf_plan_dft_r2c_2d(h, w, fft_in, (fftwf_complex *)fft_out, FFTW_ESTIMATE);
        fftwf_execute(p);

        int row, col;
        for (int y = 0; y < h; y++)
        {
            for (int i = 0; i < w; i++)
            {
                map(y, i, h, w, &row, &col);
                float v = fft_out[row * (w + pad) + col] / float(w * h);
                float norm = normalize(i, y, w, h);
                int bounded = boost(v * norm);
                image[y * w + i] = get_scaled(bounded);
            }
        }

        int bounded = round_gint((fft_out[0] / float(w * h)) - 32768.0);
        image[(h / 2) * w + (w / 2)] = get_scaled(bounded);

        delete[] fft_in;
        delete[] fft_out;

        fftwf_destroy_plan(p);
    }

    void fft_inverse(Image<uint16_t> &image)
    {
        int w = image.width();
        int h = image.height();

        int pad = (w & 1) ? 1 : 2;

        float *fft_in = new float[h * w * 2];
        float *fft_out = new float[h * w * 2];

        int row, col;
        for (int y = 0; y < h; y++)
        {
            for (int i = 0; i < w; i++)
            {
                map(y, i, h, w, &row, &col);
                float norm = normalize(i, y, w, h);
                float v = get_unscaled(image[y * w + i]);
                fft_in[row * (w + pad) + col] = unboost(v) / norm;
            }
        }

        float v = get_unscaled(image[(h / 2) * w + (w / 2)]);
        fft_in[0] = v + 32768.0;

        fftwf_plan p = fftwf_plan_dft_c2r_2d(h, w, (fftwf_complex *)fft_in, fft_out, FFTW_ESTIMATE);
        fftwf_execute(p);

        for (int y = 0; y < h; y++)
        {
            for (int i = 0; i < w; i++)
            {
                float v = fft_out[y * (w + pad - 2) + i];
                image[y * w + i] = v > 65535 ? 65535 : (v < 0 ? 0 : v);
            }
        }

        delete[] fft_in;
        delete[] fft_out;

        fftwf_destroy_plan(p);
    }
}
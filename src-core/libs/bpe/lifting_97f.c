#include <string.h>
#include <stdlib.h>
#include "bpe_internal.h"

#define F_EXTPAD 4
#define D_EXTPAD 2

/* x_alloc is no longer a static global; it is allocated in lifting_f97_2D
 * and passed as a parameter to the 1-D transform sub-functions. */

static const float LowPassFilter[] = {
     0.037828455507f,
    -0.023849465020f,
    -0.110624404418f,
     0.377402855613f,
     0.852698679009f,
     0.377402855613f,
    -0.110624404418f,
    -0.023849465020f,
     0.037828455507f
};

static const float HighPassFilter[] = {
    -0.064538882629f,
     0.040689417609f,
     0.418092273222f,
    -0.788485616406f,
     0.418092273222f,
     0.040689417609f,
    -0.064538882629f
};

static void forwardf97f(float *x_in, int N, float *x_alloc)
{
    int    i, n, half;
    float *x;
    float *d;
    float *r;

    x = x_alloc + F_EXTPAD;
    memcpy(x, x_in, sizeof(float) * N);

    half = (N >> 1);
    d    = malloc(sizeof(float) * (half + 3));
    r    = malloc(sizeof(float) * (half + 2));

    for (i = 1; i <= F_EXTPAD; i++) {
        x[-i]          = x[i];
        x[(N - 1) + i] = x[(N - 1) - i];
    }

    {
        float const *LPF = LowPassFilter  + 4;
        float const *HPF = HighPassFilter + 3;

        for (n = 0; n < half; n++) {
            d[n] = (float)(
                  LPF[0] * x[2 * n]
                + LPF[1] * (x[2 * n - 1] + x[2 * n + 1])
                + LPF[2] * (x[2 * n - 2] + x[2 * n + 2])
                + LPF[3] * (x[2 * n - 3] + x[2 * n + 3])
                + LPF[4] * (x[2 * n - 4] + x[2 * n + 4]));

            r[n] = (float)(
                  HPF[0] * x[2 * n + 1]
                + HPF[1] * (x[2 * n]     + x[2 * n + 2])
                + HPF[2] * (x[2 * n - 1] + x[2 * n + 3])
                + HPF[3] * (x[2 * n - 2] + x[2 * n + 4]));
        }
    }

    memcpy(x_in,        d, sizeof(float) * half);
    memcpy(x_in + half, r, sizeof(float) * half);

    free(d);
    free(r);
}

static void inversef97f(float *x, int N, float *x_alloc)
{
    int    i, n, half;
    float *r, *d;

    half = N / 2;
    r    = x_alloc + D_EXTPAD;
    d    = x_alloc + D_EXTPAD + half + D_EXTPAD + D_EXTPAD;
    memcpy(r, x,        half * sizeof(float));
    memcpy(d, x + half, half * sizeof(float));

    for (i = 1; i <= D_EXTPAD; i++) {
        r[-i]             = r[i];
        r[(half - 1) + i] = r[half - i];
        d[-i]             = d[i - 1];
        d[(half - 1) + i] = d[(half - 1) - i];
    }

    for (n = half; n--;) {
        *x++ = (float)(0.788486f   * r[0]  - 0.0406894f * (r[1]  + r[-1])
                     - 0.023849f   * (d[1]  + d[-2])
                     + 0.377403f   * (d[0]  + d[-1]));

        *x++ = (float)(0.418092f   * (r[1] + r[0])
                     - 0.0645389f  * (r[2]  + r[-1])
                     - 0.037829f   * (d[2]  + d[-2])
                     + 0.110624f   * (d[1]  + d[-1])
                     - 0.852699f   * d[0]);
        d++;
        r++;
    }
}

void lifting_f97_2D(float **rows,
                    int    ImgRows,
                    int    ImgCols,
                    int    levels,
                    BOOL   inverse)
{
    int    x, y, w, h, l, i;
    float *buffer;
    float *x_alloc;

    if (ImgCols % (1 << levels) || ImgRows % (1 << levels))
        ErrorMsg(BPE_FILE_ERROR);

    x_alloc = malloc(sizeof(float) * (ImgCols + ImgRows + F_EXTPAD + F_EXTPAD));
    if (!x_alloc)
        ErrorMsg(BPE_MEM_ERROR);
    for (i = 0; i < ImgCols + ImgRows + F_EXTPAD + F_EXTPAD; i++)
        x_alloc[i] = 0.0f;

    buffer = calloc(ImgRows, sizeof(float));
    if (!buffer) {
        free(x_alloc);
        ErrorMsg(BPE_MEM_ERROR);
    }

    if (!inverse) {
        for (l = 0; l < levels; l++) {
            w = (ImgCols >> l);
            h = (ImgRows >> l);
            for (y = 0; y < h; y++)
                forwardf97f(rows[y], w, x_alloc);
            for (x = 0; x < w; x++) {
                for (y = 0; y < h; y++)
                    buffer[y] = rows[y][x];
                forwardf97f(buffer, h, x_alloc);
                for (y = 0; y < h; y++)
                    rows[y][x] = buffer[y];
            }
        }
    } else {
        for (l = levels - 1; l >= 0; l--) {
            w = (ImgCols >> l);
            h = (ImgRows >> l);
            for (x = 0; x < w; x++) {
                for (y = 0; y < h; y++)
                    buffer[y] = rows[y][x];
                inversef97f(buffer, h, x_alloc);
                for (y = 0; y < h; y++)
                    rows[y][x] = buffer[y];
            }
            for (y = 0; y < h; y++)
                inversef97f(rows[y], w, x_alloc);
        }
    }

    free(x_alloc);
    free(buffer);
}

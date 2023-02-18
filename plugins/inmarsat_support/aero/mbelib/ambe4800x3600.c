/*
 * Copyright (C) 2010 mbelib Author
 * GPG Key ID: 0xEA5EFE2C (9E7A 5527 9CDC EBF7 BF1B  D772 4F98 E863 EA5E FE2C)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#define _USE_MATH_DEFINES
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "mbelib.h"
#include "ambe4800x3600_const.h"



void
mbe_dumpAmbe4800Data (char *ambe_d)
{

    int i;
    char *ambe;

    ambe = ambe_d;
    for (i = 0; i < 96; i++)
    {
        printf ("%i", *ambe);
        ambe++;
    }
    printf (" ");
}

void
mbe_dumpAmbe4800x3600Frame (char ambe_fr[6][24])
{

    int i,j;

    // c0 golay23
    printf ("ambe_fr c0: ");
    for (j = 23; j >= 0; j--)
    {
        printf ("%i", ambe_fr[0][j]);
    }
    printf (" ");
    // c1 .. c3 hamming1511

    printf ("ambe_fr c1: ");

    for (i = 1; i < 4; i++)
    {
        for (j = 14; j >= 0; j--)
        {
            printf ("%i", ambe_fr[i][j]);
        }
    }
    printf (" ");

    // c4 unprotected block
    printf ("ambe_fr c3: ");
    for (j = 14; j >= 0; j--)
    {
        printf ("%i", ambe_fr[4][j]);
    }
    printf (" ");

    // c5 unprotected block
    printf ("ambe_fr c3: ");
    for (j = 13; j >= 0; j--)
    {
        printf ("%i", ambe_fr[5][j]);
    }
    printf (" ");
}

int
mbe_eccAmbe4800x3600C0 (char ambe_fr[6][24])
{

    int j, errs;
    char in[23], out[23];

    for (j = 0; j < 23; j++)
    {
        in[j] = ambe_fr[0][j + 1];
    }
    errs = mbe_golay2312(in, out);
    // ambe_fr[0][0] should be the C0 golay24 parity bit.
    // TODO: actually test that here...
    for (j = 0; j < 23; j++)
    {
        ambe_fr[0][j + 1] = out[j];
    }

    return (errs);
}

int
mbe_eccAmbe4800x3600Data (char ambe_fr[6][24], char *ambe_d)
{

    int j;
    int errs=0;
    char *ambe, hin[15], hout[15];
    ambe = ambe_d;

    // just copy C0
    for (j = 23; j > 11; j--)
      {
        *ambe = ambe_fr[0][j];
        ambe++;
      }


    for (int i = 1; i < 4; i++)
    {
        for (j = 0; j < 15; j++)
        {
            hin[j] = ambe_fr[i][j];
        }
        errs += mbe_hamming1511 (hin, hout);
        for (j = 14; j >= 4; j--)
        {
            *ambe = hout[j];
            ambe++;
        }
    }

    // just copy C2
    for (j = 12; j >= 0; j--)
    {
        *ambe = ambe_fr[4][j];
        ambe++;
    }

    // just copy C3
    for (j = 13; j >= 0; j--)
    {
        *ambe = ambe_fr[5][j];
        ambe++;
    }

    return (errs);
}

int
mbe_decodeAmbe3600Parms (char *ambe_d, mbe_parms * cur_mp, mbe_parms * prev_mp)
{

    int ji, i, j, k, l, L,L9, m, am, ak, vl;
    int intkl[57];
    int b0, b2;
    float f0, Cik[9][18], flokl[57], deltal[57];
    float Sum42, Sum43, Tl[57], Gm[9], Ri[9], sum, c1, c2;
    int silence;
    float deltaGamma, BigGamma;
    float unvc;

    const int *bo1, *bo2;
    char bb[58][12];
    char tmpstr[13];

    silence = 0;


    // copy repeat from prev_mp
    cur_mp->repeat = prev_mp->repeat;

    // decode fundamental frequency w0 from b0
    b0 = 0;
    b0 |= ambe_d[0]<<6;
    b0 |= ambe_d[1]<<5;
    b0 |= ambe_d[2]<<4;
    b0 |= ambe_d[3]<<3;
    b0 |= ambe_d[4]<<2;
    b0 |= ambe_d[5]<<1;
    b0 |= ambe_d[71];

    if (b0 >>1 == 0x35) {

         return 3;
    }

    if ((b0 >>1 > 0x34)) {

        // Silence Frame;
        cur_mp->w0 = ((float) 2 * M_PI) / (float) 32;
        f0 = (float) 1 / (float) 16;
        L = 14;
        cur_mp->L = 14;
        for (l = 1; l <= L; l++)
        {
            cur_mp->Vl[l] = 0;
        }

        return 2;
    }

    if (silence == 0) {

        cur_mp->w0 = ((float) (4 * M_PI) / (float) ((float) (2 * b0) + 39.5));

        f0 = powf(2,(-4.311767578125 - (3.7336e-2 * ((float) b0 + 0.5))));

        cur_mp->L = (int) (0.9254 * (10 + (b0>>1)));
    }

    // hardlimit L within 9-56 range
    if (cur_mp->L < 9) {
        cur_mp->L = 9;
    } else if (cur_mp->L > 56) {
        cur_mp->L = 56;
    }

    L9 = cur_mp->L-9;
    unvc = (float)0.2046 / sqrtf(cur_mp->w0);


    // read bits from ambe_d into b1..bL-1
    bo1 = bitorder[L9][0];
    bo2 = bo1 + 1;
    for (i = 6; i < 71; i++)
    {
        bb[*bo1][*bo2] = ambe_d[i];
        bo1 += 2;
        bo2 += 2;
    }

    // Vl
    j = 0;
    int c = cur_mp->L/8;
    k = (8 - 1);
    for (i = 1; i <= cur_mp->L; i++)
    {
        cur_mp->Vl[i] = bb[1][k];
        if (j == c)
        {
            j = 0;
            if (k > 0)
            {
                k--;
            }
            else
            {
                k = 0;
            }
        }
        else
        {
            j++;
        }
    }


    float rho = 0.0;
    if (cur_mp->L <= 15) {
        rho = 0.3;
    } else if (cur_mp->L <= 24) {
        rho = 0.03 * ((float) cur_mp->L) - 0.05;

    } else {
        rho = 0.8;

    }
    //decode  b2
    tmpstr[6] = 0;
    tmpstr[0] = bb[2][5] + 48;
    tmpstr[1] = bb[2][4] + 48;
    tmpstr[2] = bb[2][3] + 48;
    tmpstr[3] = bb[2][2] + 48;
    tmpstr[4] = bb[2][1] + 48;
    tmpstr[5] = bb[2][0] + 48;
    b2 = strtol (tmpstr, NULL, 2);

    Gm[1] = 0;

    // lower the gain a little, alternatively do it in mbelib float to short
    deltaGamma = (0.42)*B2Aero[b2];
    cur_mp->gamma = deltaGamma + ((float) 0.5 * prev_mp->gamma);


    // load the 7 group average DCT's
    int ba1 = 0;
    int bm = 0;
    float x = 0;
    for (i = 0; i<7; i++)
    {
        ba1=0;
        bm = bitalloc[L9][i];

        for (j = ((int) bm - 1); j >= 0; j--)
        {
            ba1 = ba1 ^ bb[i + 3][j];

            if(j>0) ba1 <<=1;
        }
        if(bm <= 4)
        {
            x = fxlookup[lookupoffset[bm]+ba1];
        } else
        {
            x = (float) ba1 - powf(2, (bm - 1));
            if(x < 0) x-=0.5; else x+=0.5;
        }
        x = x * dq_quantstep[bm -1] * dq_prba_mul[i]
                + (dq_prba_add[i] * (1 - rho));
        Gm[i+2] = x;
    }

    // inverse DCT the gain vector
    for (i = 1; i <= 8; i++)
    {
        sum = 0;
        for (m = 1; m <= 8; m++)
        {
            if (m == 1)
            {
                am = 1;
            }
            else
            {
                am = 2;
            }
            sum = sum + ((float) am * Gm[m] * cosf ((M_PI * (float) (m - 1) * ((float) i - 0.5)) / (float) 8));
        }
        Ri[i] = sum;
    }

    // load Ci,k
    m = 7;
    for (i = 1; i <= 8; i++)
    {
        Cik[i][1] = Ri[i];

        vl = AmbeAeroLmprbl[L9][i-1];


        for (k = 2; k <= vl; k++)
        {

            ba1 = 0;
            bm = bitalloc[L9][m];
            m++;

            for (j = ((int) bm - 1); j >= 0; j--)
            {
                ba1 = ba1 ^ bb[2+m][j];

                if(j>0) ba1 <<=1;
            }
           if (bm == 0)
            {
                Cik[i][k] = 0;
            }
            else
            {

                if(bm <= 4)
                {
                    x = fxlookup[lookupoffset[bm]+ba1];
                } else
                {
                    x = (float) ba1 - powf(2, (bm - 1));
                    if(x < 0) x-=0.5; else x+=0.5;
                }
                x = x * dq_quantstep[bm -1] * dq_hoc_mul[i-1][k-2]
                        + (dq_hoc_add[i-1][k-2] * (1 - rho));
                Cik[i][k] = x;
            }

        }
    }

    // inverse DCT each block
    l = 1;
    for (i = 1; i <= 8; i++)
    {
        ji = AmbeAeroLmprbl[L9][i-1];
        for (j = 1; j <= ji; j++)
        {
            sum = 0;
            for (k = 1; k <= ji; k++)
            {
                if (k == 1)
                {
                    ak = 1;
                }
                else
                {
                    ak = 2;
                }
                sum = sum + ((float) ak * Cik[i][k] * cosf ((M_PI * (float) (k - 1) * ((float) j - 0.5)) / (float) ji));
            }
            Tl[l] = sum;
            l++;
        }
    }


    // fix for when L > L(-1)
    if (cur_mp->L > prev_mp->L)
    {
        for (l = (prev_mp->L) + 1; l <= cur_mp->L; l++)
        {
            prev_mp->Ml[l] = prev_mp->Ml[prev_mp->L];
            prev_mp->log2Ml[l] = prev_mp->log2Ml[prev_mp->L];
        }
    }
    prev_mp->log2Ml[0] = prev_mp->log2Ml[1];
    prev_mp->Ml[0] = prev_mp->Ml[1];

    Sum43 = 0;
    for (l = 1; l <= cur_mp->L; l++)
    {

        // eq. 40
        flokl[l] = ((float) prev_mp->L / (float) cur_mp->L) * (float) l;
        intkl[l] = (int) (flokl[l]);
        // eq. 41
        deltal[l] = flokl[l] - (float) intkl[l];
        // eq 43
        Sum43 = Sum43 + ((((float) 1 - deltal[l]) * prev_mp->log2Ml[intkl[l]]) + (deltal[l] * prev_mp->log2Ml[intkl[l] + 1]));
    }
    Sum43 = (((float) 0.65 / (float) cur_mp->L) * Sum43);


    // Part 2
    Sum42 = 0;
    for (l = 1; l <= cur_mp->L; l++)
    {
        Sum42 += Tl[l];
    }
    Sum42 = Sum42 / (float) cur_mp->L;
    BigGamma = cur_mp->gamma - ((float) 0.5 * (log ((float) cur_mp->L) / log ((float) 2))) - Sum42;

    // Part 3
    for (l = 1; l <= cur_mp->L; l++)
    {
        c1 = ((float) 0.65 * ((float) 1 - deltal[l]) * prev_mp->log2Ml[intkl[l]]);
        c2 = ((float) 0.65 * deltal[l] * prev_mp->log2Ml[intkl[l] + 1]);
        cur_mp->log2Ml[l] = Tl[l] + c1 + c2 - Sum43 + BigGamma;
        // inverse log to generate spectral amplitudes
        if (cur_mp->Vl[l] == 1)
        {
            cur_mp->Ml[l] = exp ((float) 0.693 * cur_mp->log2Ml[l]);
        }
        else
        {
            cur_mp->Ml[l] = unvc * exp ((float) 0.693 * cur_mp->log2Ml[l]);
        }
    }
    return (0);
}

void
mbe_demodulateAmbe4800x3600Data (char ambe_fr[6][24])
{
    int i, j, k;
    unsigned short pr[115];
    unsigned short foo = 0;

    // create pseudo-random modulator
    for (i = 23; i >= 12; i--)
    {
        foo <<= 1;
        foo |= ambe_fr[0][i];
    }
    pr[0] = (16 * foo);
    for (i = 1; i < 115; i++)
    {
        pr[i] = (173 * pr[i - 1]) + 13849 - (65536 * (((173 * pr[i - 1]) + 13849) / 65536));
    }
    for (i = 1; i < 115; i++)
    {
        pr[i] = pr[i] / 32768;
    }

    // demodulate ambe_fr with pr
    k = 1;
    for (i = 1; i < 4; i++)
    {
        for (j = 14; j >= 0; j--)
        {
            ambe_fr[i][j] = ((ambe_fr[i][j]) ^ pr[k]);
            k++;
        }
    }
}

void
mbe_processAmbe3600Dataf (float *aout_buf, int *errs, int *errs2, char *err_str, char ambe_d[72], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality)
{

    int i, bad;

    for (i = 0; i < *errs2; i++)
    {
        *err_str = '=';
        err_str++;
    }

    bad = mbe_decodeAmbe3600Parms (ambe_d, cur_mp, prev_mp);
    if (bad == 2)
    {
        // Erasure frame
        *err_str = 'E';
        err_str++;
        cur_mp->repeat = 0;
    }
    else if (bad == 3)
    {
        // Tone Frame
        *err_str = 'T';
        err_str++;
        cur_mp->repeat = 0;
    }
    else if (*errs2 > 3)
    {
        mbe_useLastMbeParms (cur_mp, prev_mp);
        cur_mp->repeat++;
        *err_str = 'R';
        err_str++;
    }
    else
    {
        cur_mp->repeat = 0;
    }

    if (bad == 0)
    {
        if (cur_mp->repeat <= 3)
        {
            mbe_moveMbeParms (cur_mp, prev_mp);
            mbe_spectralAmpEnhance (cur_mp);
            mbe_synthesizeSpeechf (aout_buf, cur_mp, prev_mp_enhanced, uvquality);
            mbe_moveMbeParms (cur_mp, prev_mp_enhanced);
        }
        else
        {
            *err_str = 'M';
            err_str++;
            mbe_synthesizeSilencef (aout_buf);
            mbe_initMbeParms (cur_mp, prev_mp, prev_mp_enhanced);
        }
    }
    else
    {
        mbe_synthesizeSilencef (aout_buf);
        mbe_initMbeParms (cur_mp, prev_mp, prev_mp_enhanced);
    }
    *err_str = 0;
}

void
mbe_processAmbe3600Data (short *aout_buf, int *errs, int *errs2, char *err_str, char ambe_d[72], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality)
{
    float float_buf[160];

    mbe_processAmbe3600Dataf (float_buf, errs, errs2, err_str, ambe_d, cur_mp, prev_mp, prev_mp_enhanced, uvquality);
    mbe_floattoshort (float_buf, aout_buf);
}

void
mbe_processAmbe4800x3600Framef (float *aout_buf, int *errs, int *errs2, char *err_str, char ambe_fr[6][24], char ambe_d[72], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality)
{

    *errs = 0;
    *errs2 = 0;
    *errs = mbe_eccAmbe4800x3600C0 (ambe_fr);
    mbe_demodulateAmbe4800x3600Data (ambe_fr);
    *errs2 = *errs;
    *errs2 += mbe_eccAmbe4800x3600Data (ambe_fr, ambe_d);

    mbe_processAmbe3600Dataf (aout_buf, errs, errs2, err_str, ambe_d, cur_mp, prev_mp, prev_mp_enhanced, uvquality);
}

void
mbe_processAmbe4800x3600Frame (short *aout_buf, int *errs, int *errs2, char *err_str, char ambe_fr[6][24], char ambe_d[72], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality)
{
    float float_buf[160];

    mbe_processAmbe4800x3600Framef (float_buf, errs, errs2, err_str, ambe_fr, ambe_d, cur_mp, prev_mp, prev_mp_enhanced, uvquality);
    mbe_floattoshort (float_buf, aout_buf);
}

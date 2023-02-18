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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "mbelib.h"
#include "mbelib_const.h"

/**
 * \return A pseudo-random float between [0.0, 1.0].
 * See http://www.azillionmonkeys.com/qed/random.html for further improvements
 */
static float
mbe_rand()
{
  return ((float) rand () / (float) RAND_MAX);
}

/**
 * \return A pseudo-random float between [-pi, +pi].
 */
static float
mbe_rand_phase()
{
  return mbe_rand() * (((float)M_PI) * 2.0F) - ((float)M_PI);
}

void
mbe_printVersion (char *str)
{
  sprintf (str, "%s", MBELIB_VERSION);
}

void
mbe_moveMbeParms (mbe_parms * cur_mp, mbe_parms * prev_mp)
{

  int l;

  prev_mp->w0 = cur_mp->w0;
  prev_mp->L = cur_mp->L;
  prev_mp->K = cur_mp->K;       // necessary?
  prev_mp->Ml[0] = (float) 0;
  prev_mp->gamma = cur_mp->gamma;
  prev_mp->repeat = cur_mp->repeat;
  for (l = 0; l <= 56; l++)
    {
      prev_mp->Ml[l] = cur_mp->Ml[l];
      prev_mp->Vl[l] = cur_mp->Vl[l];
      prev_mp->log2Ml[l] = cur_mp->log2Ml[l];
      prev_mp->PHIl[l] = cur_mp->PHIl[l];
      prev_mp->PSIl[l] = cur_mp->PSIl[l];
    }
}

void
mbe_useLastMbeParms (mbe_parms * cur_mp, mbe_parms * prev_mp)
{

  int l;

  cur_mp->w0 = prev_mp->w0;
  cur_mp->L = prev_mp->L;
  cur_mp->K = prev_mp->K;       // necessary?
  cur_mp->Ml[0] = (float) 0;
  cur_mp->gamma = prev_mp->gamma;
  cur_mp->repeat = prev_mp->repeat;
  for (l = 0; l <= 56; l++)
    {
      cur_mp->Ml[l] = prev_mp->Ml[l];
      cur_mp->Vl[l] = prev_mp->Vl[l];
      cur_mp->log2Ml[l] = prev_mp->log2Ml[l];
      cur_mp->PHIl[l] = prev_mp->PHIl[l];
      cur_mp->PSIl[l] = prev_mp->PSIl[l];
    }
}

void
mbe_initMbeParms (mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced)
{

  int l;

  prev_mp->w0 = 0.09378;
  prev_mp->L = 30;
  prev_mp->K = 10;
  prev_mp->gamma = (float) 0;
  for (l = 0; l <= 56; l++)
    {
      prev_mp->Ml[l] = (float) 0;
      prev_mp->Vl[l] = 0;
      prev_mp->log2Ml[l] = (float) 0;   // log2 of 1 == 0
      prev_mp->PHIl[l] = (float) 0;
      prev_mp->PSIl[l] = (M_PI / (float) 2);
    }
  prev_mp->repeat = 0;
  mbe_moveMbeParms (prev_mp, cur_mp);
  mbe_moveMbeParms (prev_mp, prev_mp_enhanced);
}

void
mbe_spectralAmpEnhance (mbe_parms * cur_mp)
{

  float Rm0, Rm1, R2m0, R2m1, Wl[57];
  int l;
  float sum, gamma, M;

  Rm0 = 0;
  Rm1 = 0;
  for (l = 1; l <= cur_mp->L; l++)
    {
      Rm0 = Rm0 + (cur_mp->Ml[l] * cur_mp->Ml[l]);
      Rm1 = Rm1 + ((cur_mp->Ml[l] * cur_mp->Ml[l]) * cosf (cur_mp->w0 * (float) l));
    }

  R2m0 = (Rm0*Rm0);
  R2m1 = (Rm1*Rm1);

  for (l = 1; l <= cur_mp->L; l++)
    {
      if (cur_mp->Ml[l] != 0)
        {
          Wl[l] = sqrtf (cur_mp->Ml[l]) * powf ((((float) 0.96 * M_PI * ((R2m0 + R2m1) - ((float) 2 * Rm0 * Rm1 * cosf (cur_mp->w0 * (float) l)))) / (cur_mp->w0 * Rm0 * (R2m0 - R2m1))), (float) 0.25);

          if ((8 * l) <= cur_mp->L)
            {
            }
          else if (Wl[l] > 1.2)
            {
              cur_mp->Ml[l] = 1.2 * cur_mp->Ml[l];
            }
          else if (Wl[l] < 0.5)
            {
              cur_mp->Ml[l] = 0.5 * cur_mp->Ml[l];
            }
          else
            {
              cur_mp->Ml[l] = Wl[l] * cur_mp->Ml[l];
            }
        }
    }

  // generate scaling factor
  sum = 0;
  for (l = 1; l <= cur_mp->L; l++)
    {
      M = cur_mp->Ml[l];
      if (M < 0)
        {
          M = -M;
        }
      sum += (M*M);
    }
  if (sum == 0)
    {
      gamma = (float) 1.0;
    }
  else
    {
      gamma = sqrtf (Rm0 / sum);
    }

  // apply scaling factor
  for (l = 1; l <= cur_mp->L; l++)
    {
      cur_mp->Ml[l] = gamma * cur_mp->Ml[l];
    }
}

void
mbe_synthesizeSilencef (float *aout_buf)
{

  int n;
  float *aout_buf_p;

  aout_buf_p = aout_buf;
  for (n = 0; n < 160; n++)
    {
      *aout_buf_p = (float) 0;
      aout_buf_p++;
    }
}

void
mbe_synthesizeSilence (short *aout_buf)
{

  int n;
  short *aout_buf_p;

  aout_buf_p = aout_buf;
  for (n = 0; n < 160; n++)
    {
      *aout_buf_p = (short) 0;
      aout_buf_p++;
    }
}

void
mbe_synthesizeSpeechf (float *aout_buf, mbe_parms * cur_mp, mbe_parms * prev_mp, int uvquality)
{

  int i, l, n, maxl;
  float *Ss, loguvquality;
  float C1, C2, C3, C4;
  float deltaphil, deltawl, thetaln, aln;
  int numUv;
  float cw0, pw0, cw0l, pw0l;
  float uvsine, uvrand, uvthreshold, uvthresholdf;
  float uvstep, uvoffset;
  float qfactor;
  float rphase[64], rphase2[64];

  const int N = 160;

  uvthresholdf = (float) 2700;
  uvthreshold = ((uvthresholdf * M_PI) / (float) 4000);

  // voiced/unvoiced/gain settings
  uvsine = (float) 1.3591409 *M_E;
  uvrand = (float) 2.0;

  if ((uvquality < 1) || (uvquality > 64))
    {
      printf ("\nmbelib: Error - uvquality must be within the range 1 - 64, setting to default value of 3\n");
      uvquality = 3;
    }

  // calculate loguvquality
  if (uvquality == 1)
    {
      loguvquality = (float) 1 / M_E;
    }
  else
    {
      loguvquality = log ((float) uvquality) / (float) uvquality;
    }

  // calculate unvoiced step and offset values
  uvstep = (float) 1.0 / (float) uvquality;
  qfactor = loguvquality;
  uvoffset = (uvstep * (float) (uvquality - 1)) / (float) 2;

  // count number of unvoiced bands
  numUv = 0;
  for (l = 1; l <= cur_mp->L; l++)
    {
      if (cur_mp->Vl[l] == 0)
        {
          numUv++;
        }
    }

  cw0 = cur_mp->w0;
  pw0 = prev_mp->w0;

  // init aout_buf
  Ss = aout_buf;
  for (n = 0; n < N; n++)
    {
      *Ss = (float) 0;
      Ss++;
    }

  // eq 128 and 129
  if (cur_mp->L > prev_mp->L)
    {
      maxl = cur_mp->L;
      for (l = prev_mp->L + 1; l <= maxl; l++)
        {
          prev_mp->Ml[l] = (float) 0;
          prev_mp->Vl[l] = 1;
        }
    }
  else
    {
      maxl = prev_mp->L;
      for (l = cur_mp->L + 1; l <= maxl; l++)
        {
          cur_mp->Ml[l] = (float) 0;
          cur_mp->Vl[l] = 1;
        }
    }

  // update phil from eq 139,140
  for (l = 1; l <= 56; l++)
    {
      cur_mp->PSIl[l] = prev_mp->PSIl[l] + ((pw0 + cw0) * ((float) (l * N) / (float) 2));
      if (l <= (int) (cur_mp->L / 4))
        {
          cur_mp->PHIl[l] = cur_mp->PSIl[l];
        }
      else
        {
          cur_mp->PHIl[l] = cur_mp->PSIl[l] + ((numUv * mbe_rand_phase()) / cur_mp->L);
        }
    }

  for (l = 1; l <= maxl; l++)
    {
      cw0l = (cw0 * (float) l);
      pw0l = (pw0 * (float) l);
      if ((cur_mp->Vl[l] == 0) && (prev_mp->Vl[l] == 1))
        {
          Ss = aout_buf;
          // init random phase
          for (i = 0; i < uvquality; i++)
            {
              rphase[i] = mbe_rand_phase();
            }
          for (n = 0; n < N; n++)
            {
              C1 = 0;
              // eq 131
              C1 = Ws[n + N] * prev_mp->Ml[l] * cosf ((pw0l * (float) n) + prev_mp->PHIl[l]);
              C3 = 0;
              // unvoiced multisine mix
              for (i = 0; i < uvquality; i++)
                {
                  C3 = C3 + cosf ((cw0 * (float) n * ((float) l + ((float) i * uvstep) - uvoffset)) + rphase[i]);
                  if (cw0l > uvthreshold)
                    {
                      C3 = C3 + ((cw0l - uvthreshold) * uvrand * mbe_rand());
                    }
                }
              C3 = C3 * uvsine * Ws[n] * cur_mp->Ml[l] * qfactor;
              *Ss = *Ss + C1 + C3;
              Ss++;
            }
        }
      else if ((cur_mp->Vl[l] == 1) && (prev_mp->Vl[l] == 0))
        {
          Ss = aout_buf;
          // init random phase
          for (i = 0; i < uvquality; i++)
            {
              rphase[i] = mbe_rand_phase();
            }
          for (n = 0; n < N; n++)
            {
              C1 = 0;
              // eq 132
              C1 = Ws[n] * cur_mp->Ml[l] * cosf ((cw0l * (float) (n - N)) + cur_mp->PHIl[l]);
              C3 = 0;
              // unvoiced multisine mix
              for (i = 0; i < uvquality; i++)
                {
                  C3 = C3 + cosf ((pw0 * (float) n * ((float) l + ((float) i * uvstep) - uvoffset)) + rphase[i]);
                  if (pw0l > uvthreshold)
                    {
                      C3 = C3 + ((pw0l - uvthreshold) * uvrand * mbe_rand());
                    }
                }
              C3 = C3 * uvsine * Ws[n + N] * prev_mp->Ml[l] * qfactor;
              *Ss = *Ss + C1 + C3;
              Ss++;
            }
        }
//      else if (((cur_mp->Vl[l] == 1) || (prev_mp->Vl[l] == 1)) && ((l >= 8) || (fabsf (cw0 - pw0) >= ((float) 0.1 * cw0))))
      else if ((cur_mp->Vl[l] == 1) || (prev_mp->Vl[l] == 1))
        {
          Ss = aout_buf;
          for (n = 0; n < N; n++)
            {
              C1 = 0;
              // eq 133-1
              C1 = Ws[n + N] * prev_mp->Ml[l] * cosf ((pw0l * (float) n) + prev_mp->PHIl[l]);
              C2 = 0;
              // eq 133-2
              C2 = Ws[n] * cur_mp->Ml[l] * cosf ((cw0l * (float) (n - N)) + cur_mp->PHIl[l]);
              *Ss = *Ss + C1 + C2;
              Ss++;
            }
        }
/*
      // expensive and unnecessary?
      else if ((cur_mp->Vl[l] == 1) || (prev_mp->Vl[l] == 1))
        {
          Ss = aout_buf;
          // eq 137
          deltaphil = cur_mp->PHIl[l] - prev_mp->PHIl[l] - (((pw0 + cw0) * (float) (l * N)) / (float) 2);
          // eq 138
          deltawl = ((float) 1 / (float) N) * (deltaphil - ((float) 2 * M_PI * (int) ((deltaphil + M_PI) / (M_PI * (float) 2))));
          for (n = 0; n < N; n++)
            {
              // eq 136
              thetaln = prev_mp->PHIl[l] + ((pw0l + deltawl) * (float) n) + (((cw0 - pw0) * ((float) (l * n * n)) / (float) (2 * N)));
              // eq 135
              aln = prev_mp->Ml[l] + (((float) n / (float) N) * (cur_mp->Ml[l] - prev_mp->Ml[l]));
              // eq 134
              *Ss = *Ss + (aln * cosf (thetaln));
              Ss++;
            }
        }
*/
      else
        {
          Ss = aout_buf;
          // init random phase
          for (i = 0; i < uvquality; i++)
            {
              rphase[i] = mbe_rand_phase();
            }
          // init random phase
          for (i = 0; i < uvquality; i++)
            {
              rphase2[i] = mbe_rand_phase();
            }
          for (n = 0; n < N; n++)
            {
              C3 = 0;
              // unvoiced multisine mix
              for (i = 0; i < uvquality; i++)
                {
                  C3 = C3 + cosf ((pw0 * (float) n * ((float) l + ((float) i * uvstep) - uvoffset)) + rphase[i]);
                  if (pw0l > uvthreshold)
                    {
                      C3 = C3 + ((pw0l - uvthreshold) * uvrand * mbe_rand());
                    }
                }
              C3 = C3 * uvsine * Ws[n + N] * prev_mp->Ml[l] * qfactor;
              C4 = 0;
              // unvoiced multisine mix
              for (i = 0; i < uvquality; i++)
                {
                  C4 = C4 + cosf ((cw0 * (float) n * ((float) l + ((float) i * uvstep) - uvoffset)) + rphase2[i]);
                  if (cw0l > uvthreshold)
                    {
                      C4 = C4 + ((cw0l - uvthreshold) * uvrand * mbe_rand());
                    }
                }
              C4 = C4 * uvsine * Ws[n] * cur_mp->Ml[l] * qfactor;
              *Ss = *Ss + C3 + C4;
              Ss++;
            }
        }
    }
}

void
mbe_synthesizeSpeech (short *aout_buf, mbe_parms * cur_mp, mbe_parms * prev_mp, int uvquality)
{
  float float_buf[160];

  mbe_synthesizeSpeechf (float_buf, cur_mp, prev_mp, uvquality);
  mbe_floattoshort (float_buf, aout_buf);
}

void
mbe_floattoshort (float *float_buf, short *aout_buf)
{

  short *aout_buf_p;
  float *float_buf_p;
  int i, again;
  float audio;

  again = 7;
  aout_buf_p = aout_buf;
  float_buf_p = float_buf;
  for (i = 0; i < 160; i++)
    {
      audio = again * *float_buf_p;
      if (audio > 32760)
        {
#ifdef MBE_DEBUG
          printf ("audio clip: %f\n", audio);
#endif
          audio = 32760;
        }
      else if (audio < -32760)
        {
#ifdef MBE_DEBUG
          printf ("audio clip: %f\n", audio);
#endif
          audio = -32760;
        }
      *aout_buf_p = (short) (audio);
      aout_buf_p++;
      float_buf_p++;
    }
}

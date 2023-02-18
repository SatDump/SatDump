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

#ifndef _MBELIB_H
#define _MBELIB_H

#define MBELIB_VERSION "1.3.0"

struct mbe_parameters
{
  float w0;
  int L;
  int K;
  int Vl[57];
  float Ml[57];
  float log2Ml[57];
  float PHIl[57];
  float PSIl[57];
  float gamma;
  int un;
  int repeat;
};

typedef struct mbe_parameters mbe_parms;

/*
 * Prototypes from ecc.c
 */
void mbe_checkGolayBlock (long int *block);
int mbe_golay2312 (char *in, char *out);
int mbe_hamming1511 (char *in, char *out);
int mbe_7100x4400hamming1511 (char *in, char *out);

/*
 * Prototypes from ambe3600x2400.c
 */
int mbe_eccAmbe3600x2400C0 (char ambe_fr[4][24]);
int mbe_eccAmbe3600x2400Data (char ambe_fr[4][24], char *ambe_d);
int mbe_decodeAmbe2400Parms (char *ambe_d, mbe_parms * cur_mp, mbe_parms * prev_mp);
void mbe_demodulateAmbe3600x2400Data (char ambe_fr[4][24]);
void mbe_processAmbe2400Dataf (float *aout_buf, int *errs, int *errs2, char *err_str, char ambe_d[49], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);
void mbe_processAmbe2400Data (short *aout_buf, int *errs, int *errs2, char *err_str, char ambe_d[49], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);
void mbe_processAmbe3600x2400Framef (float *aout_buf, int *errs, int *errs2, char *err_str, char ambe_fr[4][24], char ambe_d[49], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);
void mbe_processAmbe3600x2400Frame (short *aout_buf, int *errs, int *errs2, char *err_str, char ambe_fr[4][24], char ambe_d[49], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);

/*
 * Prototypes from ambe3600x2450.c
 */
int mbe_eccAmbe3600x2450C0 (char ambe_fr[4][24]);
int mbe_eccAmbe3600x2450Data (char ambe_fr[4][24], char *ambe_d);
int mbe_decodeAmbe2450Parms (char *ambe_d, mbe_parms * cur_mp, mbe_parms * prev_mp);
void mbe_demodulateAmbe3600x2450Data (char ambe_fr[4][24]);
void mbe_processAmbe2450Dataf (float *aout_buf, int *errs, int *errs2, char *err_str, char ambe_d[49], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);
void mbe_processAmbe2450Data (short *aout_buf, int *errs, int *errs2, char *err_str, char ambe_d[49], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);
void mbe_processAmbe3600x2450Framef (float *aout_buf, int *errs, int *errs2, char *err_str, char ambe_fr[4][24], char ambe_d[49], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);
void mbe_processAmbe3600x2450Frame (short *aout_buf, int *errs, int *errs2, char *err_str, char ambe_fr[4][24], char ambe_d[49], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);

/*
 * Prototypes from imbe7200x4400.c
 */
void mbe_dumpImbe4400Data (char *imbe_d);
void mbe_dumpImbe7200x4400Data (char *imbe_d);
void mbe_dumpImbe7200x4400Frame (char imbe_fr[8][23]);
int mbe_eccImbe7200x4400C0 (char imbe_fr[8][23]);
int mbe_eccImbe7200x4400Data (char imbe_fr[8][23], char *imbe_d);
int mbe_decodeImbe4400Parms (char *imbe_d, mbe_parms * cur_mp, mbe_parms * prev_mp);
void mbe_demodulateImbe7200x4400Data (char imbe[8][23]);
void mbe_processImbe4400Dataf (float *aout_buf, int *errs, int *errs2, char *err_str, char imbe_d[88], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);
void mbe_processImbe4400Data (short *aout_buf, int *errs, int *errs2, char *err_str, char imbe_d[88], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);
void mbe_processImbe7200x4400Framef (float *aout_buf, int *errs, int *errs2, char *err_str, char imbe_fr[8][23], char imbe_d[88], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);
void mbe_processImbe7200x4400Frame (short *aout_buf, int *errs, int *errs2, char *err_str, char imbe_fr[8][23], char imbe_d[88], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);

/*
 * Prototypes from imbe7100x4400.c
 */
void mbe_dumpImbe7100x4400Data (char *imbe_d);
void mbe_dumpImbe7100x4400Frame (char imbe_fr[7][24]);
int mbe_eccImbe7100x4400C0 (char imbe_fr[7][24]);
int mbe_eccImbe7100x4400Data (char imbe_fr[7][24], char *imbe_d);
void mbe_demodulateImbe7100x4400Data (char imbe[7][24]);
void mbe_convertImbe7100to7200 (char *imbe_d);
void mbe_processImbe7100x4400Framef (float *aout_buf, int *errs, int *errs2, char *err_str, char imbe_fr[7][24], char imbe_d[88], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);
void mbe_processImbe7100x4400Frame (short *aout_buf, int *errs, int *errs2, char *err_str, char imbe_fr[7][24], char imbe_d[88], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);


/*
 * Prototypes from ambe4800x3600.c
 */
int mbe_eccAmbe4800x3600C0 (char ambe_fr[4][24]);
int mbe_eccAmbe4800x3600Data (char ambe_fr[4][24], char *ambe_d);
int mbe_decodeAmbe3600Parms (char *ambe_d, mbe_parms * cur_mp, mbe_parms * prev_mp);
void mbe_demodulateAmbe4800x3600Data (char ambe_fr[4][24]);
void mbe_processAmbe3600Dataf (float *aout_buf, int *errs, int *errs2, char *err_str, char ambe_d[49], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);
void mbe_processAmbe3600Data (short *aout_buf, int *errs, int *errs2, char *err_str, char ambe_d[49], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);
void mbe_processAmbe4800x3600Framef (float *aout_buf, int *errs, int *errs2, char *err_str, char ambe_fr[6][24], char ambe_d[72], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);
void mbe_processAmbe4800x3600Frame (short *aout_buf, int *errs, int *errs2, char *err_str, char ambe_fr[6][24], char ambe_d[72], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);



/*
 * Prototypes from mbelib.c
 */
void mbe_printVersion (char *str);
void mbe_moveMbeParms (mbe_parms * cur_mp, mbe_parms * prev_mp);
void mbe_useLastMbeParms (mbe_parms * cur_mp, mbe_parms * prev_mp);
void mbe_initMbeParms (mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced);
void mbe_spectralAmpEnhance (mbe_parms * cur_mp);
void mbe_synthesizeSilencef (float *aout_buf);
void mbe_synthesizeSilence (short *aout_buf);
void mbe_synthesizeSpeechf (float *aout_buf, mbe_parms * cur_mp, mbe_parms * prev_mp, int uvquality);
void mbe_synthesizeSpeech (short *aout_buf, mbe_parms * cur_mp, mbe_parms * prev_mp, int uvquality);
void mbe_floattoshort (float *float_buf, short *aout_buf);

#endif

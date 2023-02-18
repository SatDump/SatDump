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

#include <math.h>
#include "ecc_const.h"

void
mbe_checkGolayBlock (long int *block)
{

  static int i, syndrome, eccexpected, eccbits, databits;
  long int mask, block_l;

  block_l = *block;

  mask = 0x400000l;
  eccexpected = 0;
  for (i = 0; i < 12; i++)
    {
      if ((block_l & mask) != 0l)
        {
          eccexpected ^= golayGenerator[i];
        }
      mask = mask >> 1;
    }
  eccbits = (int) (block_l & 0x7ffl);
  syndrome = eccexpected ^ eccbits;

  databits = (int) (block_l >> 11);
  databits = databits ^ golayMatrix[syndrome];

  *block = (long) databits;
}

int
mbe_golay2312 (char *in, char *out)
{

  int i, errs;
  long block;

  block = 0;
  for (i = 22; i >= 0; i--)
    {
      block = block << 1;
      block = block + in[i];
    }

  mbe_checkGolayBlock (&block);

  for (i = 22; i >= 11; i--)
    {
      out[i] = (block & 2048) >> 11;
      block = block << 1;
    }
  for (i = 10; i >= 0; i--)
    {
      out[i] = in[i];
    }

  errs = 0;
  for (i = 22; i >= 11; i--)
    {
      if (out[i] != in[i])
        {
          errs++;
        }
    }
  return (errs);
}

int
mbe_hamming1511 (char *in, char *out)
{
  int i, j, errs, block, syndrome, stmp, stmp2;

  errs = 0;

  block = 0;
  for (i = 14; i >= 0; i--)
    {
      block <<= 1;
      block |= in[i];
    }

  syndrome = 0;
  for (i = 0; i < 4; i++)
    {
      syndrome <<= 1;
      stmp = block;
      stmp &= hammingGenerator[i];

      stmp2 = (stmp % 2);
      for (j = 0; j < 14; j++)
        {
          stmp >>= 1;
          stmp2 ^= (stmp % 2);
        }

      syndrome |= stmp2;
    }
  if (syndrome > 0)
    {
      errs++;
      block ^= hammingMatrix[syndrome];
    }

  for (i = 14; i >= 0; i--)
    {
      out[i] = (block & 0x4000) >> 14;
      block = block << 1;
    }
  return (errs);
}

int
mbe_7100x4400hamming1511 (char *in, char *out)
{
  int i, j, errs, block, syndrome, stmp, stmp2;

  errs = 0;

  block = 0;
  for (i = 14; i >= 0; i--)
    {
      block <<= 1;
      block |= in[i];
    }

  syndrome = 0;
  for (i = 0; i < 4; i++)
    {
      syndrome <<= 1;
      stmp = block;
      stmp &= imbe7100x4400hammingGenerator[i];

      stmp2 = (stmp % 2);
      for (j = 0; j < 14; j++)
        {
          stmp >>= 1;
          stmp2 ^= (stmp % 2);
        }

      syndrome |= stmp2;
    }
  if (syndrome > 0)
    {
      errs++;
      block ^= hammingMatrix[syndrome];
    }

  for (i = 14; i >= 0; i--)
    {
      out[i] = (block & 0x4000) >> 14;
      block = block << 1;
    }
  return (errs);
}

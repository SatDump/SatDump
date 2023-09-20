/*
 * jdarith.c
 *
 * Copyright (C) 1991-1998, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file holds place for arithmetic entropy decoding routines.
 */

#define JPEG_INTERNALS
#include "jinclude12.h"
#include "jpeglib12.h"


/*
 * Module initialization routine for arithmetic entropy decoding.
 */
GLOBAL(void)
jinit_arith_decoder (j_decompress_ptr cinfo);

GLOBAL(void)
jinit_arith_decoder (j_decompress_ptr cinfo)
{
  ERREXIT(cinfo, JERR_ARITH_NOTIMPL);
}

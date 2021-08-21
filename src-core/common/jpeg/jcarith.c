/*
 * jcarith.c
 *
 * Copyright (C) 1991-1998, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file holds place for arithmetic entropy encoding routines.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/*
 * Module initialization routine for arithmetic entropy encoding.
 */
GLOBAL(void)
jinit_arith_encoder (j_compress_ptr cinfo);

GLOBAL(void)
jinit_arith_encoder (j_compress_ptr cinfo)
{
  ERREXIT(cinfo, JERR_ARITH_NOTIMPL);
}

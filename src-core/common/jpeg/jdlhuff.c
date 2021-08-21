/*
 * jdlhuff.c
 *
 * Copyright (C) 1991-1998, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains Huffman entropy decoding routines for lossless JPEG.
 *
 * Much of the complexity here has to do with supporting input suspension.
 * If the data source module demands suspension, we want to be able to back
 * up to the start of the current MCU.  To do this, we copy state variables
 * into local working storage, and update them back to the permanent
 * storage only upon successful completion of an MCU.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jlossls.h"       /* Private declarations for lossless codec */
#include "jdhuff.h"        /* Declarations shared with jd*huff.c */


#ifdef D_LOSSLESS_SUPPORTED

typedef struct {
  int ci, yoffset, MCU_width;
} lhd_output_ptr_info;

/*
 * Private entropy decoder object for lossless Huffman decoding.
 */

typedef struct {
  huffd_common_fields;      /* Fields shared with other entropy decoders */

  /* Pointers to derived tables (these workspaces have image lifespan) */
  d_derived_tbl * derived_tbls[NUM_HUFF_TBLS];

  /* Precalculated info set up by start_pass for use in decode_mcus: */

  /* Pointers to derived tables to be used for each data unit within an MCU */
  d_derived_tbl * cur_tbls[D_MAX_DATA_UNITS_IN_MCU];

  /* Pointers to the proper output difference row for each group of data units
   * within an MCU.  For each component, there are Vi groups of Hi data units.
   */
  JDIFFROW output_ptr[D_MAX_DATA_UNITS_IN_MCU];

  /* Number of output pointers in use for the current MCU.  This is the sum
   * of all Vi in the MCU.
   */
  int num_output_ptrs;

  /* Information used for positioning the output pointers within the output
   * difference rows.
   */
  lhd_output_ptr_info output_ptr_info[D_MAX_DATA_UNITS_IN_MCU];

  /* Index of the proper output pointer for each data unit within an MCU */
  int output_ptr_index[D_MAX_DATA_UNITS_IN_MCU];

} lhuff_entropy_decoder;

typedef lhuff_entropy_decoder * lhuff_entropy_ptr;


/*
 * Initialize for a Huffman-compressed scan.
 */

METHODDEF(void)
start_pass_lhuff_decoder (j_decompress_ptr cinfo)
{
  j_lossless_d_ptr losslsd = (j_lossless_d_ptr) cinfo->codec;
  lhuff_entropy_ptr entropy = (lhuff_entropy_ptr) losslsd->entropy_private;
  int ci, dctbl, sampn, ptrn, yoffset, xoffset;
  jpeg_component_info * compptr;

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    dctbl = compptr->dc_tbl_no;
    /* Make sure requested tables are present */
    if (dctbl < 0 || dctbl >= NUM_HUFF_TBLS ||
    cinfo->dc_huff_tbl_ptrs[dctbl] == NULL)
      ERREXIT1(cinfo, JERR_NO_HUFF_TABLE, dctbl);
    /* Compute derived values for Huffman tables */
    /* We may do this more than once for a table, but it's not expensive */
    jpeg_make_d_derived_tbl(cinfo, TRUE, dctbl,
                & entropy->derived_tbls[dctbl]);
  }

  /* Precalculate decoding info for each sample in an MCU of this scan */
  for (sampn = 0, ptrn = 0; sampn < cinfo->data_units_in_MCU;) {
    compptr = cinfo->cur_comp_info[cinfo->MCU_membership[sampn]];
    ci = compptr->component_index;
    for (yoffset = 0; yoffset < compptr->MCU_height; yoffset++, ptrn++) {
      /* Precalculate the setup info for each output pointer */
      entropy->output_ptr_info[ptrn].ci = ci;
      entropy->output_ptr_info[ptrn].yoffset = yoffset;
      entropy->output_ptr_info[ptrn].MCU_width = compptr->MCU_width;
      for (xoffset = 0; xoffset < compptr->MCU_width; xoffset++, sampn++) {
    /* Precalculate the output pointer index for each sample */
    entropy->output_ptr_index[sampn] = ptrn;
    /* Precalculate which table to use for each sample */
    entropy->cur_tbls[sampn] = entropy->derived_tbls[compptr->dc_tbl_no];
      }
    }
  }
  entropy->num_output_ptrs = ptrn;

  /* Initialize bitread state variables */
  entropy->bitstate.bits_left = 0;
  entropy->bitstate.get_buffer = 0; /* unnecessary, but keeps Purify quiet */
  entropy->insufficient_data = FALSE;
}


/*
 * Figure F.12: extend sign bit.
 * On some machines, a shift and add will be faster than a table lookup.
 */

#ifdef AVOID_TABLES

#define HUFF_EXTEND(x,s)  ((x) < (1<<((s)-1)) ? (x) + (((-1)<<(s)) + 1) : (x))

#else

#define HUFF_EXTEND(x,s)  ((x) < extend_test[s] ? (x) + extend_offset[s] : (x))

static const int extend_test[16] =   /* entry n is 2**(n-1) */
  { 0, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000 };

/*
 * Originally, a -1 was shifted but since shifting a negative value is
 * undefined behavior, now "~0U" (bit-wise NOT unsigned int 0) is used,
 * shifted and casted to an int. The result is the same, of course.
 */
static const int extend_offset[16] = /* entry n is (-1 << n) + 1 */
  { 0, (int)((~0U)<<1) + 1, (int)((~0U)<<2) + 1, (int)((~0U)<<3) + 1, (int)((~0U)<<4) + 1,
    (int)((~0U)<<5) + 1, (int)((~0U)<<6) + 1, (int)((~0U)<<7) + 1, (int)((~0U)<<8) + 1,
    (int)((~0U)<<9) + 1, (int)((~0U)<<10) + 1, (int)((~0U)<<11) + 1, (int)((~0U)<<12) + 1,
    (int)((~0U)<<13) + 1, (int)((~0U)<<14) + 1, (int)((~0U)<<15) + 1 };

#endif /* AVOID_TABLES */


/*
 * Check for a restart marker & resynchronize decoder.
 * Returns FALSE if must suspend.
 */

METHODDEF(jboolean)
process_restart (j_decompress_ptr cinfo)
{
  j_lossless_d_ptr losslsd = (j_lossless_d_ptr) cinfo->codec;
  lhuff_entropy_ptr entropy = (lhuff_entropy_ptr) losslsd->entropy_private;
  /* int ci; */

  /* Throw away any unused bits remaining in bit buffer; */
  /* include any full bytes in next_marker's count of discarded bytes */
  cinfo->marker->discarded_bytes += (unsigned int)entropy->bitstate.bits_left / 8;
  entropy->bitstate.bits_left = 0;

  /* Advance past the RSTn marker */
  if (! (*cinfo->marker->read_restart_marker) (cinfo))
    return FALSE;

  /* Reset out-of-data flag, unless read_restart_marker left us smack up
   * against a marker.  In that case we will end up treating the next data
   * segment as empty, and we can avoid producing bogus output pixels by
   * leaving the flag set.
   */
  if (cinfo->unread_marker == 0)
    entropy->insufficient_data = FALSE;

  return TRUE;
}


/*
 * Decode and return nMCU's worth of Huffman-compressed differences.
 * Each MCU is also disassembled and placed accordingly in diff_buf.
 *
 * MCU_col_num specifies the column of the first MCU being requested within
 * the MCU-row.  This tells us where to position the output row pointers in
 * diff_buf.
 *
 * Returns the number of MCUs decoded.  This may be less than nMCU if data
 * source requested suspension.  In that case no changes have been made to
 * permanent state.  (Exception: some output differences may already have
 * been assigned.  This is harmless for this module, since we'll just
 * re-assign them on the next call.)
 */

METHODDEF(JDIMENSION)
decode_mcus (j_decompress_ptr cinfo, JDIFFIMAGE diff_buf,
         JDIMENSION MCU_row_num, JDIMENSION MCU_col_num, JDIMENSION nMCU)
{
  j_lossless_d_ptr losslsd = (j_lossless_d_ptr) cinfo->codec;
  lhuff_entropy_ptr entropy = (lhuff_entropy_ptr) losslsd->entropy_private;
  unsigned int mcu_num;
  int sampn, ci, yoffset, MCU_width, ptrn;
  BITREAD_STATE_VARS;

  /* Set output pointer locations based on MCU_col_num */
  for (ptrn = 0; ptrn < entropy->num_output_ptrs; ptrn++) {
    ci = entropy->output_ptr_info[ptrn].ci;
    yoffset = entropy->output_ptr_info[ptrn].yoffset;
    MCU_width = entropy->output_ptr_info[ptrn].MCU_width;
    entropy->output_ptr[ptrn] =
      diff_buf[ci][MCU_row_num + (JDIMENSION)yoffset] + MCU_col_num * (JDIMENSION)MCU_width;
  }

  /*
   * If we've run out of data, zero out the buffers and return.
   * By resetting the undifferencer, the output samples will be CENTERJSAMPLE.
   *
   * NB: We should find a way to do this without interacting with the
   * undifferencer module directly.
   */
  if (entropy->insufficient_data) {
    for (ptrn = 0; ptrn < entropy->num_output_ptrs; ptrn++)
      jzero_far((void FAR *) entropy->output_ptr[ptrn],
        nMCU * (size_t)entropy->output_ptr_info[ptrn].MCU_width * SIZEOF(JDIFF));

    (*losslsd->predict_process_restart) (cinfo);
  }

  else {

    /* Load up working state */
    BITREAD_LOAD_STATE(cinfo,entropy->bitstate);

    /* Outer loop handles the number of MCU requested */

    for (mcu_num = 0; mcu_num < nMCU; mcu_num++) {

      /* Inner loop handles the samples in the MCU */
      for (sampn = 0; sampn < cinfo->data_units_in_MCU; sampn++) {
    d_derived_tbl * dctbl = entropy->cur_tbls[sampn];
    register int s, r;

    /* Section H.2.2: decode the sample difference */
    HUFF_DECODE(s, br_state, dctbl, return mcu_num, label1);
    if (s) {
      if (s == 16)  /* special case: always output 32768 */
        s = 32768;
      else {    /* normal case: fetch subsequent bits */
        CHECK_BIT_BUFFER(br_state, s, return mcu_num);
        r = GET_BITS(s);
        s = HUFF_EXTEND(r, s);
      }
    }

    /* Output the sample difference */
    *entropy->output_ptr[entropy->output_ptr_index[sampn]]++ = (JDIFF) s;
      }

      /* Completed MCU, so update state */
      BITREAD_SAVE_STATE(cinfo,entropy->bitstate);
    }
  }

 return nMCU;
}


/*
 * Module initialization routine for lossless Huffman entropy decoding.
 */

GLOBAL(void)
jinit_lhuff_decoder (j_decompress_ptr cinfo)
{
  j_lossless_d_ptr losslsd = (j_lossless_d_ptr) cinfo->codec;
  lhuff_entropy_ptr entropy;
  int i;

  entropy = (lhuff_entropy_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                SIZEOF(lhuff_entropy_decoder));
  losslsd->entropy_private = (void *) entropy;
  losslsd->entropy_start_pass = start_pass_lhuff_decoder;
  losslsd->entropy_process_restart = process_restart;
  losslsd->entropy_decode_mcus = decode_mcus;

  /* Mark tables unallocated */
  for (i = 0; i < NUM_HUFF_TBLS; i++) {
    entropy->derived_tbls[i] = NULL;
  }
}

#endif /* D_LOSSLESS_SUPPORTED */

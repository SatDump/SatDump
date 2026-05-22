#ifndef BPE_H
#define BPE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque context handle */
typedef struct CODINGPARAMETERS BpeContext;

/* Error codes */
#define BPE_OK                      0
#define BPE_STREAM_END              1
#define BPE_FILE_ERROR              2
#define BPE_STREAM_ERROR            3
#define BPE_DATA_ERROR              4
#define BPE_MEM_ERROR               5
#define BPE_BLOCKSCAN_CODING_ERROR  6
#define BPE_DYNAMICAL_RANGE_ERROR   7
#define BPE_RATE_ERROR              8
#define BPE_RATE_UNREACHABLE        9
#define BPE_WAVELET_INVALID         10
#define BPE_IMAGE_SIZE_WRONG        11
#define BPE_SCALLING_FILE_ERROR     12
#define BPE_INVALID_HEADER          13
#define BPE_INVALID_CODING_PARAMETERS 14
#define BPE_PATTERNING_CODING_ERROR 15
#define BPE_RICE_CODING_ERROR       16
#define BPE_STAGE_CODING_ERROR      17

/*
 * Create/destroy a coding context.
 * bpe_context_create initialises all parameters to sensible defaults.
 * The caller must call bpe_context_destroy when done.
 */
BpeContext *bpe_context_create(void);
void        bpe_context_destroy(BpeContext *ctx);

/*
 * Configuration – call these after bpe_context_create and before
 * bpe_encode / bpe_decode.
 *
 * Width/height are required for encoding; they are ignored for decoding
 * (the values are read from the compressed stream header).
 */
void bpe_set_image_size(BpeContext *ctx, uint32_t width, uint32_t height);

/* bits_per_pixel: target rate; 0 means encode to lossless capacity */
void bpe_set_bits_per_pixel(BpeContext *ctx, float bpp);

/* depth_bits: pixel bit depth 1-15; 0 means 16-bit */
void bpe_set_pixel_depth(BpeContext *ctx, uint8_t depth_bits);

/* is_signed: 0 = unsigned pixels, 1 = signed pixels */
void bpe_set_signed_pixels(BpeContext *ctx, int is_signed);

/* lsb_first: 0 = MSB first (big-endian pixels), 1 = LSB first (little-endian) */
void bpe_set_byte_order(BpeContext *ctx, int lsb_first);

/* use_integer: 1 = integer 9/7 wavelet (default), 0 = floating-point 9/7 */
void bpe_set_wavelet_type(BpeContext *ctx, int use_integer);

/* blocks: segment size in 8×8 blocks (default 256) */
void bpe_set_segment_size(BpeContext *ctx, uint32_t blocks);

/* dc_stop: 1 = stop after DC coding */
void bpe_set_dc_stop(BpeContext *ctx, int dc_stop);

/* stage_stop: 0-3; 3 = all stages (default) */
void bpe_set_stage_stop(BpeContext *ctx, int stage_stop);

/* bitplane_stop: 0-31; 0 = all bit planes (default) */
void bpe_set_bitplane_stop(BpeContext *ctx, int bitplane_stop);

/* opt: 1 = optimum k selection (default) */
void bpe_set_opt_dc_select(BpeContext *ctx, int opt);
void bpe_set_opt_ac_select(BpeContext *ctx, int opt);

/* max_bytes: maximum total compressed output size in bytes; 0 = no limit.
 * Overrides any bpe_set_bits_per_pixel() value; the actual per-pixel rate is
 * derived from max_bytes after the encoder has computed the padded image
 * dimensions (FullRows × FullCols).  Use this when you need a hard byte
 * budget rather than a per-pixel target rate. */
void bpe_set_max_output_bytes(BpeContext *ctx, size_t max_bytes);

/*
 * Encode raw pixels to a BPE compressed stream.
 *
 * pixels_in      : pointer to the raw pixel buffer (row-major, tightly packed)
 * pixels_in_size : byte size of pixels_in (must match width*height*bpp_bytes)
 * out_data       : receives a malloc'd pointer to the compressed output;
 *                  the caller must free() it.
 * out_size       : receives the number of valid bytes in *out_data
 *
 * Returns BPE_OK on success, a BPE_* error code on failure.
 * On failure *out_data is NULL and *out_size is 0.
 */
int bpe_encode(BpeContext       *ctx,
               const void       *pixels_in,
               size_t            pixels_in_size,
               uint8_t         **out_data,
               size_t           *out_size);

/*
 * Decode a BPE compressed stream back to raw pixels.
 *
 * compressed_in   : pointer to the compressed data buffer
 * compressed_size : byte size of compressed_in
 * out_pixels      : receives a malloc'd pointer to the decoded pixel buffer;
 *                   the caller must free() it.
 * out_size        : receives the number of valid bytes in *out_pixels
 *
 * Returns BPE_OK on success, a BPE_* error code on failure.
 * On failure *out_pixels is NULL and *out_size is 0.
 */
int bpe_decode(BpeContext       *ctx,
               const uint8_t    *compressed_in,
               size_t            compressed_size,
               void            **out_pixels,
               size_t           *out_size);

/*
 * Query image properties after a successful bpe_decode() call.
 * These reflect the values read from the compressed stream header,
 * which may differ from what was set on the context before decoding.
 *
 * bpe_get_width       : image width in pixels
 * bpe_get_height      : image height in pixels (pad rows excluded)
 * bpe_get_pixel_depth : bit depth per pixel (1-15; 0 means 16-bit)
 * bpe_get_signed      : 1 if pixels are signed, 0 if unsigned
 */
uint32_t bpe_get_width      (const BpeContext *ctx);
uint32_t bpe_get_height     (const BpeContext *ctx);
uint8_t  bpe_get_pixel_depth(const BpeContext *ctx);
int      bpe_get_signed     (const BpeContext *ctx);

/* Human-readable string for an error code */
const char *bpe_error_string(int err);

#ifdef __cplusplus
}
#endif

#endif /* BPE_H */

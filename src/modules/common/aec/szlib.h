#ifndef SZLIB_H
#define SZLIB_H 1

#include "libaec.h"

#define SZ_ALLOW_K13_OPTION_MASK         1
#define SZ_CHIP_OPTION_MASK              2
#define SZ_EC_OPTION_MASK                4
#define SZ_LSB_OPTION_MASK               8
#define SZ_MSB_OPTION_MASK              16
#define SZ_NN_OPTION_MASK               32
#define SZ_RAW_OPTION_MASK             128

#define SZ_OK AEC_OK
#define SZ_OUTBUFF_FULL 2

#define SZ_NO_ENCODER_ERROR -1
#define SZ_PARAM_ERROR AEC_CONF_ERROR
#define SZ_MEM_ERROR AEC_MEM_ERROR

#define SZ_MAX_PIXELS_PER_BLOCK 32
#define SZ_MAX_BLOCKS_PER_SCANLINE 128
#define SZ_MAX_PIXELS_PER_SCANLINE                              \
    (SZ_MAX_BLOCKS_PER_SCANLINE) * (SZ_MAX_PIXELS_PER_BLOCK)

typedef struct SZ_com_t_s
{
    int options_mask;
    int bits_per_pixel;
    int pixels_per_block;
    int pixels_per_scanline;
} SZ_com_t;

LIBAEC_DLL_EXPORTED int SZ_BufftoBuffCompress(
    void *dest, size_t *destLen,
    const void *source, size_t sourceLen,
    SZ_com_t *param);
LIBAEC_DLL_EXPORTED int SZ_BufftoBuffDecompress(
    void *dest, size_t *destLen,
    const void *source, size_t sourceLen,
    SZ_com_t *param);

LIBAEC_DLL_EXPORTED int SZ_encoder_enabled(void);

#endif /* SZLIB_H */

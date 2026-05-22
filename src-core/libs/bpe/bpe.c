#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "bpe_internal.h"
#include "bpe.h"

extern void DWT_(StructCodingPara *, int **, int **);
extern void HeaderUpdate(HeaderStruct *);
extern void HeaderInilization(StructCodingPara *);
extern void HeaderOutput(StructCodingPara *);
extern void HeaderReadin(StructCodingPara *);
extern void DCEncoding(StructCodingPara *, long **, BitPlaneBits *);
extern void ACBpeEncoding(StructCodingPara *, BitPlaneBits *);
extern short DCDeCoding(StructCodingPara *, StructFreBlockString *, BitPlaneBits *);
extern void ACBpeDecoding(StructCodingPara *, BitPlaneBits *);
extern void AdjustOutPut(StructCodingPara *, BitPlaneBits *);
extern void CoeffDegroup(int **, int, int);
extern void CoeffDegroupFloating(float **, int, int);
extern void DWT_Reverse(int **, StructCodingPara *);
extern void DWT_ReverseFloating(float **, StructCodingPara *);
extern void _bpe_set_ctx(StructCodingPara *ctx);

/* -------------------------------------------------------------------------
 * Context lifecycle
 * ---------------------------------------------------------------------- */

BpeContext *bpe_context_create(void)
{
    StructCodingPara *ctx = (StructCodingPara *)calloc(1, sizeof(StructCodingPara));
    if (!ctx) return NULL;
    HeaderInilization(ctx);
    return ctx;
}

void bpe_context_destroy(BpeContext *ctx)
{
    if (!ctx) return;
    if (ctx->PtrHeader) free(ctx->PtrHeader);
    if (ctx->Bits) {
        if (ctx->Bits->buf_owned && ctx->Bits->buf)
            free(ctx->Bits->buf);
        free(ctx->Bits);
    }
    free(ctx);
}

/* -------------------------------------------------------------------------
 * Configuration setters
 * ---------------------------------------------------------------------- */

void bpe_set_image_size(BpeContext *ctx, uint32_t width, uint32_t height)
{
    ctx->ImageWidth  = width;
    ctx->ImageRows   = height;
    ctx->PtrHeader->Header.Part4.ImageWidth_20Bits = width;
}

void bpe_set_bits_per_pixel(BpeContext *ctx, float bpp)
{
    ctx->BitsPerPixel = bpp;
    if (bpp != 0) {
        ctx->PtrHeader->Header.Part2.SegByteLimit_27Bits =
            (DWORD32)(bpp * ctx->PtrHeader->Header.Part3.S_20Bits * 64 / 8);
        ctx->PtrHeader->Header.Part2.UseFill = TRUE;
    } else {
        ctx->PtrHeader->Header.Part2.SegByteLimit_27Bits = 0;
        ctx->PtrHeader->Header.Part2.UseFill             = FALSE;
    }
}

void bpe_set_pixel_depth(BpeContext *ctx, uint8_t depth_bits)
{
    ctx->PtrHeader->Header.Part4.PixelBitDepth_4Bits = depth_bits & 0x0F;
}

void bpe_set_signed_pixels(BpeContext *ctx, int is_signed)
{
    ctx->PtrHeader->Header.Part4.SignedPixels = (BOOL)(is_signed ? TRUE : FALSE);
}

void bpe_set_byte_order(BpeContext *ctx, int lsb_first)
{
    ctx->PixelByteOrder = (UCHAR8)(lsb_first ? 1 : 0);
}

void bpe_set_wavelet_type(BpeContext *ctx, int use_integer)
{
    ctx->PtrHeader->Header.Part4.DWTType = (BOOL)(use_integer ? INTEGER_WAVELET : FLOAT_WAVELET);
}

void bpe_set_segment_size(BpeContext *ctx, uint32_t blocks)
{
    ctx->PtrHeader->Header.Part3.S_20Bits = blocks;
}

void bpe_set_dc_stop(BpeContext *ctx, int dc_stop)
{
    ctx->PtrHeader->Header.Part2.DCstop = (BOOL)(dc_stop ? TRUE : FALSE);
}

void bpe_set_stage_stop(BpeContext *ctx, int stage_stop)
{
    ctx->PtrHeader->Header.Part2.StageStop_2Bits = (UCHAR8)(stage_stop & 0x3);
}

void bpe_set_bitplane_stop(BpeContext *ctx, int bitplane_stop)
{
    ctx->PtrHeader->Header.Part2.BitPlaneStop_5Bits = (UCHAR8)(bitplane_stop & 0x1F);
}

void bpe_set_opt_dc_select(BpeContext *ctx, int opt)
{
    ctx->PtrHeader->Header.Part3.OptDCSelect = (BOOL)(opt ? TRUE : FALSE);
}

void bpe_set_opt_ac_select(BpeContext *ctx, int opt)
{
    ctx->PtrHeader->Header.Part3.OptACSelect = (BOOL)(opt ? TRUE : FALSE);
}

void bpe_set_max_output_bytes(BpeContext *ctx, size_t max_bytes)
{
    ctx->MaxOutputBytes = max_bytes;
}

uint32_t bpe_get_width(const BpeContext *ctx)
{
    return ctx->ImageWidth;
}

uint32_t bpe_get_height(const BpeContext *ctx)
{
    return ctx->ImageRows - ctx->PtrHeader->Header.Part1.PadRows_3Bits;
}

uint8_t bpe_get_pixel_depth(const BpeContext *ctx)
{
    return ctx->PtrHeader->Header.Part4.PixelBitDepth_4Bits;
}

int bpe_get_signed(const BpeContext *ctx)
{
    return ctx->PtrHeader->Header.Part4.SignedPixels ? 1 : 0;
}

const char *bpe_error_string(int err)
{
    extern const char *BpeErrorMsg[];
    if (err < 0 || err > 17) return "Unknown error";
    return BpeErrorMsg[err];
}

/* -------------------------------------------------------------------------
 * Bytes-per-pixel helper
 * ---------------------------------------------------------------------- */
static int bytes_per_pixel(const StructCodingPara *ctx)
{
    UCHAR8 d = ctx->PtrHeader->Header.Part4.PixelBitDepth_4Bits;
    return (d <= 8 && d != 0) ? 1 : 2;
}

/* -------------------------------------------------------------------------
 * Encoder: load raw pixels into the 2-D int array
 * ---------------------------------------------------------------------- */
static void load_pixels(StructCodingPara *ctx, int **image, const void *pixels_in)
{
    UINT32 r, i;
    UCHAR8 depth = ctx->PtrHeader->Header.Part4.PixelBitDepth_4Bits;
    UCHAR8 machineendianness;
    unsigned long int bigendtest = 1;

    if (depth <= 8 && depth != 0) {
        if (ctx->PtrHeader->Header.Part4.SignedPixels == FALSE) {
            const UCHAR8 *src = (const UCHAR8 *)pixels_in;
            for (r = 0; r < ctx->ImageRows; r++)
                for (i = 0; i < ctx->ImageWidth; i++)
                    image[r][i] = *src++;
        } else {
            const signed char *src = (const signed char *)pixels_in;
            for (r = 0; r < ctx->ImageRows; r++)
                for (i = 0; i < ctx->ImageWidth; i++)
                    image[r][i] = *src++;
        }
    } else {
        /* 16-bit pixels */
        if (ctx->PtrHeader->Header.Part4.SignedPixels == FALSE) {
            const WORD16 *src = (const WORD16 *)pixels_in;
            for (r = 0; r < ctx->ImageRows; r++)
                for (i = 0; i < ctx->ImageWidth; i++)
                    image[r][i] = src[r * ctx->ImageWidth + i];
        } else {
            const short *src = (const short *)pixels_in;
            for (r = 0; r < ctx->ImageRows; r++)
                for (i = 0; i < ctx->ImageWidth; i++)
                    image[r][i] = src[r * ctx->ImageWidth + i];
        }
        /* Swap byte order if image and machine endianness differ */
        machineendianness = (((char *)&bigendtest)[0] == 0);
        if (ctx->PixelByteOrder != machineendianness) {
            const unsigned short MSBmask = 0xFF00;
            for (r = 0; r < ctx->ImageRows; r++)
                for (i = 0; i < ctx->ImageWidth; i++)
                    image[r][i] = (image[r][i] >> 8) +
                                  ((image[r][i] << 8) & MSBmask);
        }
    }
}

/* -------------------------------------------------------------------------
 * Decoder: serialise the decoded int image to a raw byte buffer.
 * output_rows must already be adjusted for pad rows.
 * ---------------------------------------------------------------------- */
static void store_pixels_int(StructCodingPara *ctx, int **image,
                             UCHAR8 *out, UINT32 output_rows)
{
    UINT32 r, i;
    UCHAR8 depth = ctx->PtrHeader->Header.Part4.PixelBitDepth_4Bits;
    UCHAR8 machineendianness;
    unsigned long int bigendtest = 1;

    if (depth <= 8 && depth != 0) {
        if (ctx->PtrHeader->Header.Part4.SignedPixels == FALSE) {
            for (r = 0; r < output_rows; r++)
                for (i = 0; i < ctx->ImageWidth; i++) {
                    int v = image[r][i];
                    if (v > 0xFF) v = 0xFF;
                    if (v < 0)   v = 0;
                    *out++ = (UCHAR8)v;
                }
        } else {
            for (r = 0; r < output_rows; r++)
                for (i = 0; i < ctx->ImageWidth; i++) {
                    int v = image[r][i];
                    if (v > 127)  v = 127;
                    if (v < -128) v = -128;
                    *out++ = (UCHAR8)(signed char)v;
                }
        }
    } else {
        const unsigned short MSBmask = 0xFF00;
        machineendianness = (((char *)&bigendtest)[0] == 0);

        if (ctx->PtrHeader->Header.Part4.SignedPixels == FALSE) {
            WORD16 PixelMax = (depth == 0) ? (WORD16)((1 << 16) - 1)
                                           : (WORD16)((1 << depth) - 1);
            for (r = 0; r < output_rows; r++)
                for (i = 0; i < ctx->ImageWidth; i++) {
                    int v = image[r][i];
                    if (v > PixelMax) v = PixelMax;
                    if (v < 0)        v = 0;
                    if (ctx->PixelByteOrder != machineendianness)
                        v = ((v << 8) & MSBmask) + (v >> 8);
                    *out++ = (UCHAR8)( v        & 0xFF);
                    *out++ = (UCHAR8)((v >> 8)  & 0xFF);
                }
        } else {
            WORD16 PixelMax = (depth == 0) ? (WORD16)((1 << 15) - 1)
                                           : (WORD16)((1 << (depth - 1)) - 1);
            int    PixelMin = -(int)PixelMax - 1;
            for (r = 0; r < output_rows; r++)
                for (i = 0; i < ctx->ImageWidth; i++) {
                    int v = image[r][i];
                    if (v > PixelMax) v = PixelMax;
                    if (v < PixelMin) v = PixelMin;
                    if (ctx->PixelByteOrder != machineendianness)
                        v = ((v << 8) & MSBmask) + (v >> 8);
                    *out++ = (UCHAR8)( v        & 0xFF);
                    *out++ = (UCHAR8)((v >> 8)  & 0xFF);
                }
        }
    }
}

static void store_pixels_float(StructCodingPara *ctx, float **image,
                               UCHAR8 *out, UINT32 output_rows)
{
    UINT32 r, i;
    UCHAR8 depth = ctx->PtrHeader->Header.Part4.PixelBitDepth_4Bits;
    UCHAR8 machineendianness;
    unsigned long int bigendtest = 1;

    if (depth <= 8 && depth != 0) {
        if (ctx->PtrHeader->Header.Part4.SignedPixels == FALSE) {
            for (r = 0; r < output_rows; r++)
                for (i = 0; i < ctx->ImageWidth; i++) {
                    float v = image[r][i];
                    if (v > 0xFF) v = 0xFF;
                    if (v < 0)   v = 0;
                    *out++ = (UCHAR8)(int)v;
                }
        } else {
            for (r = 0; r < output_rows; r++)
                for (i = 0; i < ctx->ImageWidth; i++) {
                    float v = image[r][i];
                    if (v > 127)  v = 127;
                    if (v < -128) v = -128;
                    *out++ = (UCHAR8)(signed char)(int)v;
                }
        }
    } else {
        const unsigned short MSBmask = 0xFF00;
        machineendianness = (((char *)&bigendtest)[0] == 0);

        if (ctx->PtrHeader->Header.Part4.SignedPixels == FALSE) {
            WORD16 PixelMax = (depth == 0) ? (WORD16)((1 << 16) - 1)
                                           : (WORD16)((1 << depth) - 1);
            for (r = 0; r < output_rows; r++)
                for (i = 0; i < ctx->ImageWidth; i++) {
                    int v = (int)image[r][i];
                    if (v > PixelMax) v = PixelMax;
                    if (v < 0)        v = 0;
                    if (ctx->PixelByteOrder != machineendianness)
                        v = ((v << 8) & MSBmask) + (v >> 8);
                    *out++ = (UCHAR8)( v        & 0xFF);
                    *out++ = (UCHAR8)((v >> 8)  & 0xFF);
                }
        } else {
            WORD16 PixelMax = (depth == 0) ? (WORD16)((1 << 15) - 1)
                                           : (WORD16)((1 << (depth - 1)) - 1);
            int    PixelMin = -(int)PixelMax - 1;
            for (r = 0; r < output_rows; r++)
                for (i = 0; i < ctx->ImageWidth; i++) {
                    int v = (int)image[r][i];
                    if (v > PixelMax) v = PixelMax;
                    if (v < PixelMin) v = PixelMin;
                    if (ctx->PixelByteOrder != machineendianness)
                        v = ((v << 8) & MSBmask) + (v >> 8);
                    *out++ = (UCHAR8)( v        & 0xFF);
                    *out++ = (UCHAR8)((v >> 8)  & 0xFF);
                }
        }
    }
}

/* -------------------------------------------------------------------------
 * Encoder flush (identical logic to SegmentBufferFlushEncoder)
 * ---------------------------------------------------------------------- */
static void segment_flush_encoder(StructCodingPara *StrCoding)
{
    if (StrCoding->Bits->CodeWordAlighmentBits != 0) {
        int shift = StrCoding->Bits->CodeWord_Length -
                    StrCoding->Bits->CodeWordAlighmentBits;
        BitsOutput(StrCoding, 0, shift);
    }
    if ((StrCoding->PtrHeader->Header.Part2.SegByteLimit_27Bits != 0) &&
        (StrCoding->SegmentFull == FALSE) &&
        StrCoding->PtrHeader->Header.Part2.UseFill == TRUE) {
        while (StrCoding->SegmentFull == FALSE)
            BitsOutput(StrCoding, 0, 8);
    }
    StrCoding->Bits->SegBitCounter         = 0;
    StrCoding->Bits->ByteBuffer_4Bytes     = 0;
    StrCoding->Bits->CodeWordAlighmentBits = 0;
}

/* -------------------------------------------------------------------------
 * Decoder flush (identical to SegmentBufferFlushDecoder)
 * ---------------------------------------------------------------------- */
static void segment_flush_decoder(StructCodingPara *StrCoding)
{
    DWORD32 codeWordLengthInBits, temp, remBits, padBits;

    if ((StrCoding->PtrHeader->Header.Part2.SegByteLimit_27Bits != 0) &&
        (StrCoding->SegmentFull == FALSE) &&
        StrCoding->PtrHeader->Header.Part2.UseFill == TRUE) {
        temp = 0;
        while (StrCoding->SegmentFull == FALSE)
            BitsRead(StrCoding, &temp, 8);
    }
    codeWordLengthInBits = (StrCoding->PtrHeader->Header.Part4.CodewordLength_2Bits + 1) * 8;
    remBits  = (StrCoding->Bits->TotalBitCounter) % codeWordLengthInBits;
    padBits  = remBits != 0 ? codeWordLengthInBits - remBits : 0;
    BitsRead(StrCoding, &temp, (short)padBits);
    StrCoding->Bits->TotalBitCounter       += StrCoding->Bits->CodeWordAlighmentBits;
    StrCoding->Bits->SegBitCounter          = 0;
    StrCoding->Bits->ByteBuffer_4Bytes      = 0;
    StrCoding->Bits->CodeWordAlighmentBits  = 0;
}

/* -------------------------------------------------------------------------
 * BuildBlockString: rearrange 2-D wavelet coefficients into block strings
 * ---------------------------------------------------------------------- */
static void build_block_string(int **TransformedImage,
                               int ImageRows, int ImageWidth,
                               long **BlockString)
{
    int i, j, k, p, counter = 0;
    int BlockRow = ImageRows  / BLOCK_SIZE;
    int BlockCol = ImageWidth / BLOCK_SIZE;

    for (i = 0; i < BlockRow; i++)
        for (j = 0; j < BlockCol; j++)
            for (k = 0; k < BLOCK_SIZE; k++) {
                for (p = 0; p < BLOCK_SIZE; p++)
                    BlockString[counter][p] =
                        TransformedImage[i * BLOCK_SIZE + k][j * BLOCK_SIZE + p];
                counter++;
            }
}

/* -------------------------------------------------------------------------
 * Encode engine
 * ---------------------------------------------------------------------- */
static void encode_engine(StructCodingPara *PtrCoding,
                          const void *pixels_in)
{
    int    **OriginalImage    = NULL;
    int    **TransformedImage = NULL;
    long   **BlockString      = NULL;
    UINT32   TotalBlocks      = 0;
    UINT32   i, j;
    UCHAR8   TempPaddedRows   = 0;
    BitPlaneBits *BlockCodingInfo = NULL;

    /* Validate caller has set image dimensions */
    if (PtrCoding->ImageRows == 0 || PtrCoding->ImageWidth == 0)
        ErrorMsg(BPE_IMAGE_SIZE_WRONG);

    /* Compute padding to make dimensions multiples of 8 */
    if (PtrCoding->ImageRows % BLOCK_SIZE != 0)
        PtrCoding->PtrHeader->Header.Part1.PadRows_3Bits =
            BLOCK_SIZE - (PtrCoding->ImageRows % BLOCK_SIZE);

    PtrCoding->PtrHeader->Header.Part4.ImageWidth_20Bits = PtrCoding->ImageWidth;

    if (PtrCoding->ImageWidth % BLOCK_SIZE != 0)
        PtrCoding->PadCols_3Bits = BLOCK_SIZE - (PtrCoding->ImageWidth % BLOCK_SIZE);

    UINT32 PadRows = PtrCoding->PtrHeader->Header.Part1.PadRows_3Bits;
    UINT32 PadCols = PtrCoding->PadCols_3Bits;
    UINT32 FullRows = PtrCoding->ImageRows + PadRows;
    UINT32 FullCols = PtrCoding->ImageWidth + PadCols;

    /* Allocate original and transformed image buffers */
    OriginalImage = (int **)calloc(FullRows, sizeof(int *));
    for (i = 0; i < FullRows; i++)
        OriginalImage[i] = (int *)calloc(FullCols, sizeof(int));

    /* Load pixels from caller buffer */
    load_pixels(PtrCoding, OriginalImage, pixels_in);

    /* Replicate last row and column for padding */
    for (i = 0; i < PadRows; i++)
        for (j = 0; j < FullCols; j++)
            OriginalImage[PtrCoding->ImageRows + i][j] =
                OriginalImage[PtrCoding->ImageRows - 1][j];
    for (i = 0; i < PadCols; i++)
        for (j = 0; j < FullRows; j++)
            OriginalImage[j][PtrCoding->ImageWidth + i] =
                OriginalImage[j][PtrCoding->ImageWidth - 1];

    TransformedImage = (int **)calloc(FullRows, sizeof(int *));
    for (i = 0; i < FullRows; i++)
        TransformedImage[i] = (int *)calloc(FullCols, sizeof(int));

    TotalBlocks = (FullRows / BLOCK_SIZE) * (FullCols / BLOCK_SIZE);

    /* If the caller specified a byte budget, convert it to a per-pixel rate
     * now that we know the padded image dimensions. This overrides any
     * prior bpe_set_bits_per_pixel() value. */
    if (PtrCoding->MaxOutputBytes > 0) {
        float bpp = (float)PtrCoding->MaxOutputBytes * 8.0f
                    / ((float)FullRows * (float)FullCols);
        DWORD32 seg_bytes = (DWORD32)(bpp
                            * (float)PtrCoding->PtrHeader->Header.Part3.S_20Bits
                            * 64.0f / 8.0f);
        if (seg_bytes == 0)
            seg_bytes = 1;
        if (seg_bytes > (DWORD32)((1u << 27) - 1))
            seg_bytes = (DWORD32)((1u << 27) - 1);
        PtrCoding->BitsPerPixel = bpp;
        PtrCoding->PtrHeader->Header.Part2.SegByteLimit_27Bits = seg_bytes;
        PtrCoding->PtrHeader->Header.Part2.UseFill             = TRUE;
    }

    BlockString = (long **)calloc(TotalBlocks * BLOCK_SIZE, sizeof(long *));
    for (i = 0; i < TotalBlocks * BLOCK_SIZE; i++)
        BlockString[i] = (long *)calloc(BLOCK_SIZE, sizeof(long));

    /* Initialise encoder output buffer */
    PtrCoding->Bits->buf       = NULL;
    PtrCoding->Bits->buf_size  = 0;
    PtrCoding->Bits->buf_pos   = 0;
    PtrCoding->Bits->buf_owned = 1;

    /* 1. Wavelet transform */
    DWT_(PtrCoding, OriginalImage, TransformedImage);

    /* Free OriginalImage */
    for (i = 0; i < FullRows; i++)
        free(OriginalImage[i]);
    free(OriginalImage);
    OriginalImage = NULL;

    build_block_string(TransformedImage, (int)FullRows, (int)FullCols, BlockString);

    /* Free all allocated rows */
    for (i = 0; i < FullRows; i++)
        free(TransformedImage[i]);
    free(TransformedImage);
    TransformedImage = NULL;

    TempPaddedRows = PtrCoding->PtrHeader->Header.Part1.PadRows_3Bits;
    PtrCoding->PtrHeader->Header.Part1.PadRows_3Bits = 0;

    /* 2. Segment encoding loop */
    for (; PtrCoding->BlockCounter < TotalBlocks; ) {
        if (PtrCoding->BlockCounter + PtrCoding->PtrHeader->Header.Part3.S_20Bits == TotalBlocks) {
            PtrCoding->PtrHeader->Header.Part1.EngImgFlg      = TRUE;
            PtrCoding->PtrHeader->Header.Part1.PadRows_3Bits  = TempPaddedRows;
        } else if (PtrCoding->BlockCounter + PtrCoding->PtrHeader->Header.Part3.S_20Bits > TotalBlocks) {
            PtrCoding->PtrHeader->Header.Part1.EngImgFlg      = TRUE;
            PtrCoding->PtrHeader->Header.Part1.PadRows_3Bits  = TempPaddedRows;
            PtrCoding->PtrHeader->Header.Part1.Part3Flag       = TRUE;
            PtrCoding->PtrHeader->Header.Part3.S_20Bits        = TotalBlocks - PtrCoding->BlockCounter;
            PtrCoding->PtrHeader->Header.Part1.Part2Flag       = TRUE;
            if (PtrCoding->BitsPerPixel != 0)
                PtrCoding->PtrHeader->Header.Part2.SegByteLimit_27Bits =
                    (DWORD32)(PtrCoding->BitsPerPixel *
                              PtrCoding->PtrHeader->Header.Part3.S_20Bits * 64 / 8);
            else
                PtrCoding->PtrHeader->Header.Part2.SegByteLimit_27Bits = 0; /* no byte limit */
        }

        BlockCodingInfo = (BitPlaneBits *)calloc(
            PtrCoding->PtrHeader->Header.Part3.S_20Bits, sizeof(BitPlaneBits));

        /* 2a. DC encoding */
        DCEncoding(PtrCoding, BlockString, BlockCodingInfo);

        /* 2b. AC encoding (unless DCstop or segment already full) */
        if ((PtrCoding->SegmentFull == FALSE) &&
            !(PtrCoding->PtrHeader->Header.Part2.DCstop == TRUE))
            ACBpeEncoding(PtrCoding, BlockCodingInfo);

        /* Always free BlockCodingInfo (was leaked on DCstop/SegmentFull paths) */
        free(BlockCodingInfo);
        BlockCodingInfo = NULL;

        if (PtrCoding->PtrHeader->Header.Part1.EngImgFlg == TRUE)
            break;

        PtrCoding->BlockCounter += PtrCoding->PtrHeader->Header.Part3.S_20Bits;
        HeaderUpdate(PtrCoding->PtrHeader);
        segment_flush_encoder(PtrCoding);
        PtrCoding->SegmentFull = FALSE;
    }

    segment_flush_encoder(PtrCoding);

    for (i = 0; i < TotalBlocks * BLOCK_SIZE; i++)
        free(BlockString[i]);
    free(BlockString);
}

/* -------------------------------------------------------------------------
 * Free all in-progress decoder allocations tracked in ctx->dec.
 * Called from bpe_decode()'s error handler after longjmp.
 * ---------------------------------------------------------------------- */
static void decode_cleanup(StructCodingPara *ctx)
{
    UINT32 i;

    free(ctx->dec.blkinfo);
    ctx->dec.blkinfo = NULL;

    if (ctx->dec.blockstr) {
        StructFreBlockString *node = ctx->dec.blockstr;
        /* rewind to head */
        while (node->previous)
            node = node->previous;
        /* walk forward, freeing each node */
        while (node) {
            StructFreBlockString *nxt = node->next;
            if (node->FreqBlkString) {
                for (i = 0; i < node->Blocks * BLOCK_SIZE; i++)
                    free(node->FreqBlkString[i]);
                free(node->FreqBlkString);
            }
            if (node->FloatingFreqBlk) {
                for (i = 0; i < node->Blocks * BLOCK_SIZE; i++)
                    free(node->FloatingFreqBlk[i]);
                free(node->FloatingFreqBlk);
            }
            free(node);
            node = nxt;
        }
        ctx->dec.blockstr = NULL;
    }

    if (ctx->dec.imgout_int) {
        for (i = 0; i < ctx->dec.imgout_rows; i++)
            free(ctx->dec.imgout_int[i]);
        free(ctx->dec.imgout_int);
        ctx->dec.imgout_int = NULL;
    }
    if (ctx->dec.imgout_float) {
        for (i = 0; i < ctx->dec.imgout_rows; i++)
            free(ctx->dec.imgout_float[i]);
        free(ctx->dec.imgout_float);
        ctx->dec.imgout_float = NULL;
    }
    ctx->dec.imgout_rows = 0;

    if (ctx->dec.tr_int) {
        for (i = 0; i < ctx->dec.tr_rows; i++)
            free(ctx->dec.tr_int[i]);
        free(ctx->dec.tr_int);
        ctx->dec.tr_int = NULL;
    }
    if (ctx->dec.tr_float) {
        for (i = 0; i < ctx->dec.tr_rows; i++)
            free(ctx->dec.tr_float[i]);
        free(ctx->dec.tr_float);
        ctx->dec.tr_float = NULL;
    }
    ctx->dec.tr_rows = 0;
}

/* -------------------------------------------------------------------------
 * Decode engine
 * ---------------------------------------------------------------------- */
static void decode_engine(StructCodingPara *PtrCoding,
                          const uint8_t *compressed_in, size_t compressed_size,
                          void **out_pixels, size_t *out_size)
{
    UINT32   i, j, X = 0, Y = 0, TotalBlocks = 0;
    int    **imgout_int   = NULL;
    float  **imgout_float = NULL;
    StructFreBlockString *StrFreBlockString = NULL;
    StructFreBlockString *tempStr           = NULL;
    StructFreBlockString *pStrToFree        = NULL;

    /* Point the bitstream reader at the caller's compressed buffer.
     * We do NOT own this buffer; buf_owned stays 0. */
    PtrCoding->Bits->buf       = (UCHAR8 *)compressed_in;
    PtrCoding->Bits->buf_size  = compressed_size;
    PtrCoding->Bits->buf_pos   = 0;
    PtrCoding->Bits->buf_owned = 0;

    /* Reset bit counters (context may be reused). */
    PtrCoding->Bits->ByteBuffer_4Bytes     = 0;
    PtrCoding->Bits->CodeWordAlighmentBits = 0;
    PtrCoding->Bits->SegBitCounter         = 0;
    PtrCoding->Bits->TotalBitCounter       = 0;
    PtrCoding->BlockCounter                = 0;

    HeaderReadin(PtrCoding);

    /* Validate the first header before touching any values.
     * ImageWidth need not be a multiple of BLOCK_SIZE — the encoder pads
     * columns internally and stores the original width in the header.
     * What must be a multiple of 8 is FullCols (= ImageWidth + PadCols),
     * which is guaranteed by the PadCols formula as long as ImageWidth > 0. */
    PtrCoding->ImageWidth = PtrCoding->PtrHeader->Header.Part4.ImageWidth_20Bits;
    if (PtrCoding->ImageWidth == 0)
        ErrorMsg(BPE_INVALID_HEADER);
    if (PtrCoding->PtrHeader->Header.Part3.S_20Bits < SEGMENT_S_MIN)
        ErrorMsg(BPE_INVALID_HEADER);

    PtrCoding->PadCols_3Bits = (PtrCoding->ImageWidth % BLOCK_SIZE != 0)
        ? BLOCK_SIZE - (PtrCoding->ImageWidth % BLOCK_SIZE) : 0;

    StrFreBlockString = (StructFreBlockString *)calloc(1, sizeof(StructFreBlockString));
    if (!StrFreBlockString) ErrorMsg(BPE_MEM_ERROR);
    StrFreBlockString->next     = NULL;
    StrFreBlockString->previous = NULL;
    PtrCoding->dec.blockstr     = StrFreBlockString;

    for (;;) {
        UINT32 S = PtrCoding->PtrHeader->Header.Part3.S_20Bits;

        /* Guard against garbage S_20Bits causing TotalBlocks overflow */
        if (TotalBlocks + S < TotalBlocks ||
            TotalBlocks + S > MAX_TOTAL_DECODE_BLOCKS)
            ErrorMsg(BPE_INVALID_HEADER);
        TotalBlocks += S;

        /* Set Blocks before any inner allocation so decode_cleanup can
         * correctly free partial arrays on OOM. */
        StrFreBlockString->Blocks = S;

        StrFreBlockString->FreqBlkString = (long **)calloc(
            S * BLOCK_SIZE, sizeof(long *));
        if (!StrFreBlockString->FreqBlkString) ErrorMsg(BPE_MEM_ERROR);
        for (i = 0; i < S * BLOCK_SIZE; i++) {
            StrFreBlockString->FreqBlkString[i] = (long *)calloc(BLOCK_SIZE, sizeof(long));
            if (!StrFreBlockString->FreqBlkString[i]) ErrorMsg(BPE_MEM_ERROR);
        }

        StrFreBlockString->FloatingFreqBlk = (float **)calloc(
            S * BLOCK_SIZE, sizeof(float *));
        if (!StrFreBlockString->FloatingFreqBlk) ErrorMsg(BPE_MEM_ERROR);
        for (i = 0; i < S * BLOCK_SIZE; i++) {
            StrFreBlockString->FloatingFreqBlk[i] = (float *)calloc(BLOCK_SIZE, sizeof(float));
            if (!StrFreBlockString->FloatingFreqBlk[i]) ErrorMsg(BPE_MEM_ERROR);
        }

        BitPlaneBits *BlockCodingInfo = (BitPlaneBits *)calloc(S, sizeof(BitPlaneBits));
        if (!BlockCodingInfo) ErrorMsg(BPE_MEM_ERROR);
        PtrCoding->dec.blkinfo = BlockCodingInfo;

        DCDeCoding(PtrCoding, StrFreBlockString, BlockCodingInfo);
        if (PtrCoding->PtrHeader->Header.Part2.DCstop == 0)
            ACBpeDecoding(PtrCoding, BlockCodingInfo);
        AdjustOutPut(PtrCoding, BlockCodingInfo);

        free(BlockCodingInfo);
        PtrCoding->dec.blkinfo = NULL;

        segment_flush_decoder(PtrCoding);
        PtrCoding->SegmentFull  = FALSE;
        PtrCoding->RateReached  = FALSE;
        PtrCoding->DecodingStopLocations.BitPlaneStopDecoding = 0;
        PtrCoding->BlockCounter += S;

        if (PtrCoding->PtrHeader->Header.Part1.EngImgFlg == TRUE)
            break;

        HeaderReadin(PtrCoding);

        /* Validate each subsequent segment header too */
        if (PtrCoding->PtrHeader->Header.Part3.S_20Bits < SEGMENT_S_MIN)
            ErrorMsg(BPE_INVALID_HEADER);

        tempStr = (StructFreBlockString *)calloc(1, sizeof(StructFreBlockString));
        if (!tempStr) ErrorMsg(BPE_MEM_ERROR);
        StrFreBlockString->next = tempStr;
        tempStr->previous       = StrFreBlockString;
        tempStr->next           = NULL;
        StrFreBlockString       = tempStr;
        PtrCoding->dec.blockstr = tempStr;
    }

    /* Compute total image dimensions and validate before allocating */
    UINT32 FullCols = PtrCoding->ImageWidth + PtrCoding->PadCols_3Bits;
    if (FullCols == 0) ErrorMsg(BPE_INVALID_HEADER);
    if ((TotalBlocks * (UINT32)BLOCK_SIZE * (UINT32)BLOCK_SIZE) % FullCols != 0)
        ErrorMsg(BPE_INVALID_HEADER);
    PtrCoding->ImageRows = TotalBlocks * 64 / FullCols;
    if (PtrCoding->ImageRows == 0 ||
        (PtrCoding->ImageRows % BLOCK_SIZE) != 0)
        ErrorMsg(BPE_INVALID_HEADER);

    imgout_int   = (int **)  calloc(PtrCoding->ImageRows, sizeof(int *));
    imgout_float = (float **)calloc(PtrCoding->ImageRows, sizeof(float *));
    if (!imgout_int || !imgout_float) ErrorMsg(BPE_MEM_ERROR);
    PtrCoding->dec.imgout_int   = imgout_int;
    PtrCoding->dec.imgout_float = imgout_float;
    PtrCoding->dec.imgout_rows  = PtrCoding->ImageRows;

    for (i = 0; i < PtrCoding->ImageRows; i++) {
        imgout_int[i]   = (int *)  calloc(FullCols, sizeof(int));
        imgout_float[i] = (float *)calloc(FullCols, sizeof(float));
        if (!imgout_int[i] || !imgout_float[i]) ErrorMsg(BPE_MEM_ERROR);
    }

    /* Rewind linked list to head */
    while (StrFreBlockString->previous != NULL)
        StrFreBlockString = StrFreBlockString->previous;

    /* Scatter block strings back into 2-D image arrays, then free each node.
     * (No ErrorMsg can fire here — only free() calls.) */
    do {
        UINT32 F_x = 0;
        do {
            for (i = 0; i < BLOCK_SIZE; i++)
                for (j = 0; j < BLOCK_SIZE; j++) {
                    imgout_int  [X + i][Y + j] = StrFreBlockString->FreqBlkString   [F_x + i][j];
                    imgout_float[X + i][Y + j] = StrFreBlockString->FloatingFreqBlk [F_x + i][j];
                }
            Y += BLOCK_SIZE;
            if (Y >= PtrCoding->ImageWidth) { Y = 0; X += BLOCK_SIZE; }
            F_x += BLOCK_SIZE;
        } while (F_x < StrFreBlockString->Blocks * BLOCK_SIZE);

        pStrToFree = StrFreBlockString;
        for (i = 0; i < pStrToFree->Blocks * BLOCK_SIZE; i++)
            free(pStrToFree->FreqBlkString[i]);
        free(pStrToFree->FreqBlkString);
        for (i = 0; i < pStrToFree->Blocks * BLOCK_SIZE; i++)
            free(pStrToFree->FloatingFreqBlk[i]);
        free(pStrToFree->FloatingFreqBlk);

        StrFreBlockString = StrFreBlockString->next;
        free(pStrToFree);
    } while (StrFreBlockString != NULL);
    PtrCoding->dec.blockstr = NULL;  /* list fully freed */

    /* Inverse wavelet transform (may ErrorMsg; imgout still tracked in dec) */
    if (PtrCoding->PtrHeader->Header.Part4.DWTType == INTEGER_WAVELET) {
        CoeffDegroup(imgout_int, PtrCoding->ImageRows, FullCols);
        DWT_Reverse(imgout_int, PtrCoding);
    } else {
        CoeffDegroupFloating(imgout_float, PtrCoding->ImageRows, FullCols);
        DWT_ReverseFloating(imgout_float, PtrCoding);
    }

    /* Handle optional transpose.
     *
     * The stream's ImageWidth = original image height (H), and
     * ImageRows = original image width (W) + PadRows.
     * imgout[ImageRows][ImageWidth] holds the decoded (transposed) coefficients.
     * We reverse-transpose to recover the original H×W image.
     *
     * tr is indexed tr[j][i] = imgout[i][j]:
     *   j ∈ [0, ImageWidth)  → outer dimension needs ImageWidth entries  (tw)
     *   i ∈ [0, ImageRows)   → inner dimension needs ImageRows entries   (th)
     *
     * After transpose the output has ImageWidth rows and (ImageRows-PadRows) cols.
     * PadRows is padding on the i-axis (original W), not on the j-axis (original H).
     */
    if (PtrCoding->PtrHeader->Header.Part4.TransposeImg == TRANSPOSE) {
        UINT32 PadRows = PtrCoding->PtrHeader->Header.Part1.PadRows_3Bits;
        UINT32 tw = PtrCoding->ImageWidth;   /* outer dimension: j < ImageWidth */
        UINT32 th = PtrCoding->ImageRows;    /* inner dimension: i < ImageRows  */
        if (PtrCoding->PtrHeader->Header.Part4.DWTType == INTEGER_WAVELET) {
            int **tr = (int **)calloc(tw, sizeof(int *));
            if (!tr) ErrorMsg(BPE_MEM_ERROR);
            PtrCoding->dec.tr_int  = tr;
            PtrCoding->dec.tr_rows = tw;
            for (i = 0; i < tw; i++) {
                tr[i] = (int *)calloc(th, sizeof(int));
                if (!tr[i]) ErrorMsg(BPE_MEM_ERROR);
            }
            for (i = 0; i < PtrCoding->ImageRows; i++)
                for (j = 0; j < PtrCoding->ImageWidth; j++)
                    tr[j][i] = imgout_int[i][j];
            UINT32 tmp_rows  = PtrCoding->ImageRows;
            UINT32 tmp_width = PtrCoding->ImageWidth;
            /* Output: tw rows, (th - PadRows) cols — swap ctx dims for store_pixels */
            PtrCoding->ImageRows  = tw;
            PtrCoding->ImageWidth = th - PadRows;
            UINT32 output_rows = tw;           /* no padding in the j-axis */
            *out_size   = (size_t)output_rows * (th - PadRows) * bytes_per_pixel(PtrCoding);
            *out_pixels = malloc(*out_size);
            if (!*out_pixels) ErrorMsg(BPE_MEM_ERROR);
            store_pixels_int(PtrCoding, tr, (UCHAR8 *)*out_pixels, output_rows);
            PtrCoding->ImageRows  = tmp_rows;
            PtrCoding->ImageWidth = tmp_width;
            for (i = 0; i < tw; i++) free(tr[i]);
            free(tr);
            PtrCoding->dec.tr_int  = NULL;
            PtrCoding->dec.tr_rows = 0;
        } else {
            float **tr = (float **)calloc(tw, sizeof(float *));
            if (!tr) ErrorMsg(BPE_MEM_ERROR);
            PtrCoding->dec.tr_float = tr;
            PtrCoding->dec.tr_rows  = tw;
            for (i = 0; i < tw; i++) {
                tr[i] = (float *)calloc(th, sizeof(float));
                if (!tr[i]) ErrorMsg(BPE_MEM_ERROR);
            }
            for (i = 0; i < PtrCoding->ImageRows; i++)
                for (j = 0; j < PtrCoding->ImageWidth; j++)
                    tr[j][i] = imgout_float[i][j];
            UINT32 tmp_rows  = PtrCoding->ImageRows;
            UINT32 tmp_width = PtrCoding->ImageWidth;
            PtrCoding->ImageRows  = tw;
            PtrCoding->ImageWidth = th - PadRows;
            UINT32 output_rows = tw;
            *out_size   = (size_t)output_rows * (th - PadRows) * bytes_per_pixel(PtrCoding);
            *out_pixels = malloc(*out_size);
            if (!*out_pixels) ErrorMsg(BPE_MEM_ERROR);
            store_pixels_float(PtrCoding, tr, (UCHAR8 *)*out_pixels, output_rows);
            PtrCoding->ImageRows  = tmp_rows;
            PtrCoding->ImageWidth = tmp_width;
            for (i = 0; i < tw; i++) free(tr[i]);
            free(tr);
            PtrCoding->dec.tr_float = NULL;
            PtrCoding->dec.tr_rows  = 0;
        }
    } else {
        UINT32 output_rows = PtrCoding->ImageRows -
                             PtrCoding->PtrHeader->Header.Part1.PadRows_3Bits;
        *out_size   = (size_t)output_rows * PtrCoding->ImageWidth * bytes_per_pixel(PtrCoding);
        *out_pixels = malloc(*out_size);
        if (!*out_pixels) ErrorMsg(BPE_MEM_ERROR);
        if (PtrCoding->PtrHeader->Header.Part4.DWTType == INTEGER_WAVELET)
            store_pixels_int  (PtrCoding, imgout_int,   (UCHAR8 *)*out_pixels, output_rows);
        else
            store_pixels_float(PtrCoding, imgout_float, (UCHAR8 *)*out_pixels, output_rows);
    }

    /* Clear the reference to the caller's buffer so destroy() won't free it */
    PtrCoding->Bits->buf      = NULL;
    PtrCoding->Bits->buf_size = 0;
    PtrCoding->Bits->buf_pos  = 0;

    /* Free output image arrays */
    PtrCoding->dec.imgout_int   = NULL;
    PtrCoding->dec.imgout_float = NULL;
    PtrCoding->dec.imgout_rows  = 0;
    for (i = 0; i < PtrCoding->ImageRows; i++) {
        free(imgout_int[i]);
        free(imgout_float[i]);
    }
    free(imgout_int);
    free(imgout_float);
}

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

int bpe_encode(BpeContext       *ctx,
               const void       *pixels_in,
               size_t            pixels_in_size,
               uint8_t         **out_data,
               size_t           *out_size)
{
    int rc;
    (void)pixels_in_size;  /* validated by caller; we trust it matches dimensions */

    *out_data = NULL;
    *out_size = 0;

    _bpe_set_ctx(ctx);
    rc = setjmp(ctx->err_jmp);
    if (rc != 0) {
        _bpe_set_ctx(NULL);
        /* Release any partially allocated encoder buffer */
        if (ctx->Bits->buf_owned && ctx->Bits->buf) {
            free(ctx->Bits->buf);
            ctx->Bits->buf       = NULL;
            ctx->Bits->buf_size  = 0;
            ctx->Bits->buf_pos   = 0;
            ctx->Bits->buf_owned = 0;
        }
        return rc;
    }

    encode_engine(ctx, pixels_in);

    /* Transfer ownership of the compressed buffer to the caller */
    *out_data = ctx->Bits->buf;
    *out_size = ctx->Bits->buf_pos;
    ctx->Bits->buf       = NULL;
    ctx->Bits->buf_size  = 0;
    ctx->Bits->buf_pos   = 0;
    ctx->Bits->buf_owned = 0;

    _bpe_set_ctx(NULL);
    return BPE_OK;
}

int bpe_decode(BpeContext       *ctx,
               const uint8_t    *compressed_in,
               size_t            compressed_size,
               void            **out_pixels,
               size_t           *out_size)
{
    int rc;

    *out_pixels = NULL;
    *out_size   = 0;

    /* Zero decode cleanup state so decode_cleanup() is a safe no-op
     * if called before decode_engine() allocates anything. */
    ctx->dec.blockstr     = NULL;
    ctx->dec.blkinfo      = NULL;
    ctx->dec.imgout_int   = NULL;
    ctx->dec.imgout_float = NULL;
    ctx->dec.imgout_rows  = 0;
    ctx->dec.tr_int       = NULL;
    ctx->dec.tr_float     = NULL;
    ctx->dec.tr_rows      = 0;

    _bpe_set_ctx(ctx);
    rc = setjmp(ctx->err_jmp);
    if (rc != 0) {
        _bpe_set_ctx(NULL);
        decode_cleanup(ctx);
        if (*out_pixels) { free(*out_pixels); *out_pixels = NULL; *out_size = 0; }
        ctx->Bits->buf      = NULL;
        ctx->Bits->buf_size = 0;
        ctx->Bits->buf_pos  = 0;
        return rc;
    }

    decode_engine(ctx, compressed_in, compressed_size, out_pixels, out_size);

    _bpe_set_ctx(NULL);
    return BPE_OK;
}

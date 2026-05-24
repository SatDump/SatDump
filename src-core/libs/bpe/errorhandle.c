#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <setjmp.h>
#include "bpe_internal.h"

const char *BpeErrorMsg[] = {
    "Success",
    "Error code 1: Bit stream end",
    "Error code 2: File error",
    "Error code 3: Bitstream error",
    "Error code 4: Data error",
    "Error code 5: Memory allocation error",
    "Error code 6: Decoding error",
    "Error code 7: Dynamical range error",
    "Error code 8: Invalid rate",
    "Error code 9: Cannot get the exact rate",
    "Error code 10: Wavelet transform invalid",
    "Error code 11: Invalid image segment size",
    "Error code 12: Scaling file error or invalid scales",
    "Error code 13: Invalid header",
    "Error code 14: Invalid coding parameters",
    "Error code 15: Pattern coding error",
    "Error code 16: Rice coding error",
    "Error code 17: Stage coding error"
};

/* Thread-local pointer to the currently active coding context.
 * Set by _bpe_set_ctx() before calling encode/decode; cleared after.
 * Allows ErrorMsg() to reach the context from deep call stacks that
 * do not pass StructCodingPara* (e.g. DC_EnDeCoding.c). */
#if defined(_MSC_VER)
static __declspec(thread) StructCodingPara *_bpe_current_ctx = NULL;
#else
static __thread StructCodingPara *_bpe_current_ctx = NULL;
#endif

void _bpe_set_ctx(StructCodingPara *ctx)
{
    _bpe_current_ctx = ctx;
}

void DebugInfo(char *m)
{
    (void)m;
}

void ErrorMsg(int err)
{
    if (_bpe_current_ctx) {
        _bpe_current_ctx->last_error = err;
        longjmp(_bpe_current_ctx->err_jmp, err);
    }
    /* Fallback if called outside a library context (should not happen). */
    fprintf(stderr, "BPE fatal error: %s\n", ERR_MSG(err));
    exit(err);
}

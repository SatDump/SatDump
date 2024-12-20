#pragma once

#include "bzlib/bzlib.h"

// Slightly modified version of the original,
// to return how many bytes were consumed as well,
// so we can iterate over a few concatenated files
int BZ2_bzBuffToBuffDecompress_M(char* dest,
    unsigned int* destLen,
    char* source,
    unsigned int sourceLen,
    unsigned int* consumedSource,
    int small,
    int verbosity)
{
    bz_stream strm;
    int ret;

    if (dest == NULL || destLen == NULL ||
        source == NULL ||
        (small != 0 && small != 1) ||
        verbosity < 0 || verbosity > 4)
        return BZ_PARAM_ERROR;

    strm.bzalloc = NULL;
    strm.bzfree = NULL;
    strm.opaque = NULL;
    ret = BZ2_bzDecompressInit(&strm, verbosity, small);
    if (ret != BZ_OK)
        return ret;

    strm.next_in = source;
    strm.next_out = dest;
    strm.avail_in = sourceLen;
    strm.avail_out = *destLen;

    ret = BZ2_bzDecompress(&strm);
    if (ret == BZ_OK)
        goto output_overflow_or_eof;
    if (ret != BZ_STREAM_END)
        goto errhandler;

    /* normal termination */
    *destLen -= strm.avail_out;
    *consumedSource = sourceLen - strm.avail_in;
    BZ2_bzDecompressEnd(&strm);
    return BZ_OK;

output_overflow_or_eof:
    if (strm.avail_out > 0)
    {
        BZ2_bzDecompressEnd(&strm);
        return BZ_UNEXPECTED_EOF;
    }
    else
    {
        BZ2_bzDecompressEnd(&strm);
        return BZ_OUTBUFF_FULL;
    };

errhandler:
    BZ2_bzDecompressEnd(&strm);
    return ret;
}
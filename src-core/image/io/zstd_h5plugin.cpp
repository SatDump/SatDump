#include "zstd_h5plugin.h"
#include "zstd.h"
#include <stdlib.h>

size_t zstd_filter(unsigned int flags, size_t cd_nelmts, const unsigned int cd_values[], size_t nbytes, size_t *buf_size, void **buf)
{
    void *outbuf = NULL; /* Pointer to new output buffer */
    void *inbuf = NULL;  /* Pointer to input buffer */
    inbuf = *buf;

    size_t ret_value;
    size_t origSize = nbytes; /* Number of bytes for output (compressed) buffer */

    if (flags & H5Z_FLAG_REVERSE)
    {
        size_t decompSize = ZSTD_getDecompressedSize(*buf, origSize);
        if (NULL == (outbuf = malloc(decompSize)))
            goto error;

        decompSize = ZSTD_decompress(outbuf, decompSize, inbuf, origSize);

        free(*buf);
        *buf = outbuf;
        outbuf = NULL;
        ret_value = (size_t)decompSize;
    }
    else
    {
        int aggression;
        if (cd_nelmts > 0)
            aggression = (int)cd_values[0];
        else
            aggression = ZSTD_CLEVEL_DEFAULT;
        if (aggression < 1 /*ZSTD_minCLevel()*/)
            aggression = 1 /*ZSTD_minCLevel()*/;
        else if (aggression > ZSTD_maxCLevel())
            aggression = ZSTD_maxCLevel();

        size_t compSize = ZSTD_compressBound(origSize);
        if (NULL == (outbuf = malloc(compSize)))
            goto error;

        compSize = ZSTD_compress(outbuf, compSize, inbuf, origSize, aggression);

        free(*buf);
        *buf = outbuf;
        *buf_size = compSize;
        outbuf = NULL;
        ret_value = compSize;
    }
    if (outbuf != NULL)
        free(outbuf);
    return ret_value;

error:
    return 0;
}

const H5Z_class_t zstd_H5Filter = {H5Z_CLASS_T_VERS, (H5Z_filter_t)(ZSTD_FILTER), 1, 1, "Zstandard compression: http://www.zstd.net", NULL, NULL, (H5Z_func_t)(zstd_filter)};

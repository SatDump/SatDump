#include <hdf5.h>

#define ZSTD_FILTER 32015

#ifdef __cplusplus
extern "C"
{
#endif

    size_t zstd_filter(unsigned int flags, size_t cd_nelmts, const unsigned int cd_values[], size_t nbytes, size_t *buf_size, void **buf);

    extern const H5Z_class_t zstd_H5Filter;

#ifdef __cplusplus
}
#endif
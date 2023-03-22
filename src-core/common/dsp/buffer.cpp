#define SATDUMP_DLL_EXPORT 1
#include "buffer.h"

namespace dsp
{
    // 1MB buffers
    SATDUMP_DLL int STREAM_BUFFER_SIZE = 1000000;
    SATDUMP_DLL int RING_BUF_SZ = 1000000;
};
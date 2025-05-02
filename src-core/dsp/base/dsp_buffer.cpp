#define SATDUMP_DLL_EXPORT 1
#include "dsp_buffer.h"
#include <volk/volk.h>

namespace satdump
{
    namespace ndsp
    {
        SATDUMP_DLL const size_t volk_alignment = volk_get_alignment();
    }
} // namespace satdump
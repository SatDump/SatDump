#include "dsp_buffer.h"
#include <volk/volk.h>

namespace satdump
{
    namespace ndsp
    {
        const size_t volk_alignment = volk_get_alignment();
    }
}
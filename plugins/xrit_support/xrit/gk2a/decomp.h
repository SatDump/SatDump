#pragma once

#include "common/lrit/lrit_file.h"
#include "gk2a_headers.h"

namespace satdump
{
    namespace xrit
    {
        namespace gk2a
        {
            void decompressGK2AHritFileIfRequired(::lrit::LRITFile &file);
        } // namespace gk2a
    } // namespace xrit
} // namespace satdump
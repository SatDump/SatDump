#pragma once

#include "common/lrit/lrit_file.h"
#include "msg_headers.h"

namespace satdump
{
    namespace xrit
    {
        namespace msg
        {
            void decompressMsgHritFileIfRequired(::lrit::LRITFile &file);
        } // namespace msg
    } // namespace xrit
} // namespace satdump
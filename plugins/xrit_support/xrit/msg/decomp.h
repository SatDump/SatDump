#pragma once

/**
 * @file decomp.h
 * @brief MSG/ELEKTRO Decompression functions
 */

#include "msg_headers.h"
#include "xrit/xrit_file.h"

namespace satdump
{
    namespace xrit
    {
        namespace msg
        {
            /**
             * @brief Parses headers to figure out the compression utilized
             * (usually Wavelet on HRIT and JPEG on LRIT), and decompressed the file
             * back into raw data.
             * @param file file to decompress
             */
            void decompressMsgHritFileIfRequired(XRITFile &file);
        } // namespace msg
    } // namespace xrit
} // namespace satdump
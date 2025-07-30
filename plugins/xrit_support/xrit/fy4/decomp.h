#pragma once

/**
 * @file decomp.h
 * @brief FY-4x Decompression functions
 */

#include "fy4_headers.h"
#include "xrit/xrit_file.h"

namespace satdump
{
    namespace xrit
    {
        namespace fy4
        {
            /**
             * @brief Parses headers to figure out the compression utilized
             * (usually J2K), and decompressed the file back into raw data.
             * @param file file to decompress
             */
            void decompressFY4HritFileIfRequired(XRITFile &file);
        } // namespace fy4
    } // namespace xrit
} // namespace satdump
#pragma once

/**
 * @file decomp.h
 * @brief GK-2A Decompression functions
 */

#include "gk2a_headers.h"
#include "xrit/xrit_file.h"

namespace satdump
{
    namespace xrit
    {
        namespace gk2a
        {
            /**
             * @brief Parses headers to figure out the compression utilized
             * (usually J2K on HRIT and JPEG on LRIT), and decompressed the file
             * back into raw data.
             * @param file file to decompress
             */
            void decompressGK2AHritFileIfRequired(XRITFile &file);
        } // namespace gk2a
    } // namespace xrit
} // namespace satdump
#pragma once

/**
 * @file geotiff.h
 */

#include "projection/standard/proj.h"
#include <tiffio.h>

namespace satdump
{
    namespace geotiff
    {
        /**
         * @brief Attempts to parse TIFF tags from a GeoTIFF file.
         *
         * @param proj projection to write
         * @param tif the libtiff object
         * @return true on success
         */
        bool try_read_geotiff(proj::projection_t *proj, TIFF *tif);

        /**
         * @brief Attempts to write TIFF tags to a GeoTIFF file.
         *
         * @param tif the libtiff object
         * @param proj projection to write
         * @return true on success
         */
        bool try_write_geotiff(TIFF *tif, proj::projection_t *proj);
    } // namespace geotiff
} // namespace satdump
#pragma once

#include <tiffio.h>
#include "common/projection/projs2/proj_json.h"

namespace geotiff
{
    /*
    Attempts to parse TIFF tags of a GeoTIFF file.
    Returns true on success, false on failure.
    */
    bool try_read_geotiff(proj::projection_t *proj, TIFF *tif);

    bool try_write_geotiff(TIFF *tif, proj::projection_t *proj);
}
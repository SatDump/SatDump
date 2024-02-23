#include "image.h"
#include <cstring>
#include <fstream>
#include "logger.h"
#include <filesystem>
#include <tiffio.h>

#define INVERT_ENDIAN_16(x) ((x >> 8) | (x << 8))

#include "image_meta.h"
#include "common/projection/projs2/proj_json.h"
#include "common/projection/geotiff/geotiff.h"

namespace image
{
    template <typename T>
    void Image<T>::save_tiff(std::string file)
    {
        if (data_size == 0 || d_height == 0) // Make sure we aren't just gonna crash
        {
            logger->trace("Tried to save empty TIFF!");
            return;
        }
    }

    template <typename T>
    void Image<T>::load_tiff(std::string file)
    {
        if (!std::filesystem::exists(file))
            return;

        TIFF *tif = TIFFOpen(file.c_str(), "r");
        if (tif)
        {
            uint32_t w, h;
            size_t npixels;
            uint32_t *raster;

            TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
            TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);

            proj::projection_t pro;
            if (geotiff::try_read_geotiff(&pro, tif))
            {
                nlohmann::json meta;
                meta["proj_cfg"] = pro;
                set_metadata(*this, meta);
            }

            npixels = w * h;
            raster = (uint32_t *)_TIFFmalloc(npixels * sizeof(uint32_t));

            printf("Loading TIFF\n");

            if (raster != NULL)
            {
                init(w, h, 4);

                if (TIFFReadRGBAImage(tif, w, h, raster, 0))
                {
                    // ... process raster data...

                    for (size_t x = 0; x < w; x++)
                    {
                        for (size_t y = 0; y < h; y++)
                        {
                            size_t i = y * w + x;
                            size_t i2 = ((h - 1) - y) * w + x;
                            if (d_depth == 8)
                            {
                                channel(0)[i2] = (raster[i] >> 0) & 0xFF;
                                channel(1)[i2] = (raster[i] >> 8) & 0xFF;
                                channel(2)[i2] = (raster[i] >> 16) & 0xFF;
                                channel(3)[i2] = (raster[i] >> 24) & 0xFF;
                            }
                            else if (d_depth == 16)
                            {
                                channel(0)[i2] = ((raster[i] >> 0) & 0xFF) << 8;
                                channel(1)[i2] = ((raster[i] >> 8) & 0xFF) << 8;
                                channel(2)[i2] = ((raster[i] >> 16) & 0xFF) << 8;
                                channel(3)[i2] = ((raster[i] >> 24) & 0xFF) << 8;
                            }
                        }
                    }
                }
                _TIFFfree(raster);
            }
            TIFFClose(tif);
        }

        printf("Done Loading TIFF\n");
    }

    // Generate Images for uint16_t and uint8_t
    template class Image<uint8_t>;
    template class Image<uint16_t>;
}

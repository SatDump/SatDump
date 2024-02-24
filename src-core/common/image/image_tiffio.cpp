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

        TIFF *tif = TIFFOpen(file.c_str(), "w");
        if (tif)
        {
            TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, d_width);
            TIFFSetField(tif, TIFFTAG_IMAGELENGTH, d_height);
            TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, d_channels);
            TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
            TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
            TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
            TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, d_channels == 1 ? PHOTOMETRIC_MINISBLACK : PHOTOMETRIC_RGB);
            if (d_channels == 4)
            {
                uint16_t out[1];
                out[0] = EXTRASAMPLE_ASSOCALPHA;
                TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, 1, &out);
            }

            tsize_t linebytes = d_channels * d_width * sizeof(uint8_t); // length in memory of one row of pixel in the image.

            unsigned char *buf = NULL; // buffer used to store the row of pixel information for writing to file
            //    Allocating memory to store the pixels of current row
            if (TIFFScanlineSize(tif) == linebytes)
                buf = (unsigned char *)_TIFFmalloc(linebytes);
            else
                buf = (unsigned char *)_TIFFmalloc(TIFFScanlineSize(tif));

            // We set the strip size of the file to be size of one row of pixels
            TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tif, d_width * d_channels));

            // Now writing image to the file one strip at a time
            for (size_t y = 0; y < d_height; y++)
            {
                if (d_channels == 4)
                {
                    for (size_t x = 0; x < d_width; x++)
                    {
                        size_t i2 = /*((d_height - 1) - y)*/ y * d_width + x;
                        if (d_depth == 8)
                        {
                            buf[x * 4 + 0] = channel(0)[i2];
                            buf[x * 4 + 1] = channel(1)[i2];
                            buf[x * 4 + 2] = channel(2)[i2];
                            buf[x * 4 + 3] = channel(3)[i2];
                        }
                        else if (d_depth == 16)
                        {
                            buf[x * 4 + 0] = channel(0)[i2] >> 8;
                            buf[x * 4 + 1] = channel(1)[i2] >> 8;
                            buf[x * 4 + 2] = channel(2)[i2] >> 8;
                            buf[x * 4 + 3] = channel(3)[i2] >> 8;
                        }
                    }
                }

                if (TIFFWriteScanline(tif, buf, y, 0) < 0)
                    break;
            }

            _TIFFfree(buf);

            if (image::has_metadata(*this))
            {
                nlohmann::json meta = image::get_metadata(*this);
                if (meta.contains("proj_cfg"))
                {
                    try
                    {
                        proj::projection_t proj = meta["proj_cfg"];
                        geotiff::try_write_geotiff(tif, &proj);
                    }
                    catch (std::exception &e)
                    {
                    }
                }
            }

            TIFFClose(tif);
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

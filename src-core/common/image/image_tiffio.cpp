#include "common/projection/projs2/proj_json.h"
#include "common/projection/geotiff/geotiff.h"
#include "image.h"
#include "image_meta.h"
#include <cstring>
#include <fstream>
#include "logger.h"
#include <filesystem>
#include <tiffio.h>

#define INVERT_ENDIAN_16(x) ((x >> 8) | (x << 8))

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
            TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, d_depth);
            TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
            TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
            TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, d_channels == 1 ? PHOTOMETRIC_MINISBLACK : PHOTOMETRIC_RGB);
            if (d_channels == 4)
            {
                uint16_t out[1];
                out[0] = EXTRASAMPLE_ASSOCALPHA;
                TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, 1, &out);
            }

            tsize_t linebytes = d_channels * d_width * (d_depth == 8 ? sizeof(uint8_t) : sizeof(uint16_t));

            unsigned char *buf = NULL;
            if (TIFFScanlineSize(tif) == linebytes)
                buf = (unsigned char *)_TIFFmalloc(linebytes);
            else
                buf = (unsigned char *)_TIFFmalloc(TIFFScanlineSize(tif));

            TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tif, d_width * d_channels));

            for (size_t y = 0; y < d_height; y++)
            {
                if (d_channels == 4)
                {
                    for (size_t x = 0; x < d_width; x++)
                    {
                        size_t i2 = /*((d_height - 1) - y)*/ y * d_width + x;
                        if (d_depth == 8)
                        {
                            for (int i = 0; i < d_channels; i++)
                                ((uint8_t *)buf)[x * d_channels + i] = channel(i)[i2];
                        }
                        else if (d_depth == 16)
                        {
                            for (int i = 0; i < d_channels; i++)
                                ((uint16_t *)buf)[x * d_channels + i] = channel(i)[i2];
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
            int16_t bit_depth, channels_number;
            size_t npixels;
            uint32_t *raster;

            TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
            TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
            TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &channels_number);
            TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bit_depth);

            proj::projection_t pro;
            if (geotiff::try_read_geotiff(&pro, tif))
            {
                nlohmann::json meta;
                meta["proj_cfg"] = pro;
                set_metadata(*this, meta);
            }

            if (bit_depth != 8 && bit_depth != 16)
            {
                logger->error("Unsupported TIFF bit depth %d", bit_depth);
                return;
            }

            init(w, h, channels_number);

            tsize_t linebytes = d_channels * d_width * (bit_depth == 16 ? sizeof(uint16_t) : sizeof(uint8_t));
            unsigned char *buf = NULL;
            if (TIFFScanlineSize(tif) == linebytes)
                buf = (unsigned char *)_TIFFmalloc(linebytes);
            else
                buf = (unsigned char *)_TIFFmalloc(TIFFScanlineSize(tif));

            for (size_t y = 0; y < d_height; y++)
            {
                TIFFReadScanline(tif, buf, y, 0);

                if (d_depth == 16)
                {
                    for (size_t x = 0; x < d_width; x++)
                    {
                        if (bit_depth == 8)
                        {
                            for (int i = 0; i < d_channels; i++)
                                channel(i)[y * d_width + x] = ((uint8_t *)buf)[x * channels_number + i] << 8;
                        }
                        else if (bit_depth == 16)
                        {
                            for (int i = 0; i < d_channels; i++)
                                channel(i)[y * d_width + x] = ((uint16_t *)buf)[x * channels_number + i];
                        }
                    }
                }
                else if (d_depth == 8)
                {
                    for (size_t x = 0; x < d_width; x++)
                    {
                        if (bit_depth == 8)
                        {
                            for (int i = 0; i < d_channels; i++)
                                channel(i)[y * d_width + x] = ((uint8_t *)buf)[x * channels_number + i];
                        }
                        else if (bit_depth == 16)
                        {
                            for (int i = 0; i < d_channels; i++)
                                channel(i)[y * d_width + x] = ((uint16_t *)buf)[x * channels_number + i] >> 8;
                        }
                    }
                }
            }

            _TIFFfree(buf);

            TIFFClose(tif);
        }

        printf("Done Loading TIFF\n");
    }

    // Generate Images for uint16_t and uint8_t
    template class Image<uint8_t>;
    template class Image<uint16_t>;
}

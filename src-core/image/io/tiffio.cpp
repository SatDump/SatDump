#include "../io.h"
#include "../meta.h"
#include "common/projection/geotiff/geotiff.h"
#include "common/projection/projs2/proj_json.h"
#include "logger.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <tiffio.h>

#define INVERT_ENDIAN_16(x) ((x >> 8) | (x << 8))

namespace satdump
{
    namespace image
    {
        void save_tiff(Image &img, std::string file)
        {
            auto d_depth = img.depth();
            auto d_channels = img.channels();
            auto d_height = img.height();
            auto d_width = img.width();

            if (img.size() == 0 || d_height == 0) // Make sure we aren't just gonna crash
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
                    out[0] = EXTRASAMPLE_UNASSALPHA;
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
                    for (size_t x = 0; x < d_width; x++)
                    {
                        size_t i2 = /*((d_height - 1) - y)*/ y * d_width + x;
                        if (d_depth == 8)
                        {
                            for (int i = 0; i < d_channels; i++)
                                ((uint8_t *)buf)[x * d_channels + i] = img.get(i, i2);
                        }
                        else if (d_depth == 16)
                        {
                            for (int i = 0; i < d_channels; i++)
                                ((uint16_t *)buf)[x * d_channels + i] = img.get(i, i2);
                        }
                    }

                    if (TIFFWriteScanline(tif, buf, y, 0) < 0)
                        break;
                }

                _TIFFfree(buf);

                if (image::has_metadata(img))
                {
                    nlohmann::json meta = image::get_metadata(img);
                    if (meta.contains("proj_cfg"))
                    {
                        try
                        {
                            proj::projection_t proj = meta["proj_cfg"];
                            geotiff::try_write_geotiff(tif, &proj);
                        }
                        catch (std::exception &)
                        {
                        }
                    }
                }

                TIFFClose(tif);
            }
        }

        void load_tiff(Image &img, std::string file)
        {
            if (!std::filesystem::exists(file))
                return;

            TIFF *tif = TIFFOpen(file.c_str(), "r");
            if (tif)
            {
                uint32_t w, h;
                int16_t bit_depth, channels_number;

                TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
                TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
                TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &channels_number);
                TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bit_depth);

                proj::projection_t pro;
                if (geotiff::try_read_geotiff(&pro, tif))
                {
                    nlohmann::json meta;
                    meta["proj_cfg"] = pro;
                    meta["proj_cfg"]["width"] = w;
                    meta["proj_cfg"]["height"] = h;
                    set_metadata(img, meta);
                }

                if (bit_depth != 8 && bit_depth != 16)
                {
                    logger->error("Unsupported TIFF bit depth %d", bit_depth);
                    return;
                }

                img.init(bit_depth, w, h, channels_number);

                int d_channels = channels_number;
                size_t d_width = w;
                size_t d_height = h;
                int d_depth = bit_depth;

                uint32 tile_width, tile_length;
                if (TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tile_width) && TIFFGetField(tif, TIFFTAG_TILELENGTH, &tile_length))
                {

                    unsigned char *buf = (unsigned char *)_TIFFmalloc(TIFFTileSize(tif));
                    logger->error("%d - %d - %d - %d", tile_width, tile_length, d_channels, TIFFTileSize(tif));
                    for (size_t y = 0; y < d_height; y += tile_length)
                    {
                        for (size_t x = 0; x < d_width; x += tile_width)
                        {
                            TIFFReadTile(tif, buf, x, y, 0, 0);

                            if (d_depth == 16)
                            {
                                for (size_t t_x = 0; t_x < tile_width; t_x++)
                                    for (size_t t_y = 0; t_y < tile_length; t_y++)
                                        for (int i = 0; i < d_channels; i++)
                                            if ((y + t_y) < d_height && (x + t_x) < d_width)
                                                img.set(i, (y + t_y) * d_width + (x + t_x), ((uint16_t *)buf)[(t_y * tile_width + t_x) * channels_number + i]);
                            }
                            else if (d_depth == 8)
                            {
                                for (size_t t_x = 0; t_x < tile_width; t_x++)
                                    for (size_t t_y = 0; t_y < tile_length; t_y++)
                                        for (int i = 0; i < d_channels; i++)
                                            if ((y + t_y) < d_height && (x + t_x) < d_width)
                                                img.set(i, (y + t_y) * d_width + (x + t_x), ((uint8_t *)buf)[(t_y * tile_width + t_x) * channels_number + i]);
                            }
                        }
                    }
                    _TIFFfree(buf);
                }
                else
                {
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
                                for (int i = 0; i < d_channels; i++)
                                    img.set(i, y * d_width + x, ((uint16_t *)buf)[x * channels_number + i]);
                        }
                        else if (d_depth == 8)
                        {
                            for (size_t x = 0; x < d_width; x++)
                            {
                                for (int i = 0; i < d_channels; i++)
                                    img.set(i, y * d_width + x, ((uint8_t *)buf)[x * channels_number + i]);
                            }
                        }
                    }

                    _TIFFfree(buf);
                }

                TIFFClose(tif);
            }

            printf("Done Loading TIFF\n");
        }
    } // namespace image
} // namespace satdump
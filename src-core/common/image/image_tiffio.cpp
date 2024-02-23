#include "image.h"
#include <cstring>
#include <fstream>
#include "logger.h"
#include <filesystem>
#include <tiffio.h>

#define INVERT_ENDIAN_16(x) ((x >> 8) | (x << 8))

#include "image_meta.h"
#include "common/projection/projs2/proj_json.h"

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

#define ModelTiepointTag 33922
            {
                uint32_t count;
                double *value;
                TIFFGetField(tif, ModelTiepointTag, &count, &value);

                for (int i = 0; i < count; i++)
                    printf("%f, ", value[i]);
                printf("\n");

                if (count >= 6)
                {
                    pro.proj_offset_x = value[3];
                    pro.proj_offset_y = value[4];
                }
            }

#define ModelPixelScaleTag 33550
            {
                uint32_t count;
                double *value;
                if (TIFFGetField(tif, ModelPixelScaleTag, &count, &value))
                {
                    printf("SIZE %d\n", count);
                    for (int i = 0; i < count; i++)
                        printf("%f, ", value[i]);
                    printf("\n");

                    if (count >= 2)
                    {
                        pro.proj_scalar_x = value[0];
                        pro.proj_scalar_y = -value[1];
                    }
                }
            }

#define GeoKeyDirectoryTag 34735
            {
                {
                    uint32_t count;
                    uint16_t *value;
                    TIFFGetField(tif, GeoKeyDirectoryTag, &count, &value);

                    for (int i = 0; i < count; i++)
                        printf("%d, ", value[i]);
                    printf("\n");

                    printf("GeoKeyDirectoryTag :\n");
                    printf(" - KeyDirectoryVersion : %d\n", value[0]);
                    printf(" - KeyRevision : %d\n", value[1]);
                    printf(" - MinorRevision : %d\n", value[2]);
                    printf(" - NumberOfKeys : %d\n", value[3]);

                    for (int i = 0; i < value[3]; i++)
                    {
                        int KeyID = value[4 + i * 4 + 0];
                        int TIFFTagLocation = value[4 + i * 4 + 1];
                        int Count = value[4 + i * 4 + 2];
                        int Value_Offset = value[4 + i * 4 + 3];

                        if (TIFFTagLocation == 0)
                        {
                            const char *GTModelTypeGeoKey_v[] = {"Unknown", "ModelTypeProjected", "ModelTypeGeographic", "ModelTypeGeocentric"};
                            const char *ProjCoordTransGeoKey_v[] = {"Unknown", "CT_TransverseMercator", "CT_TransvMercator_Modified_Alaska", "CT_ObliqueMercator", "CT_ObliqueMercator_Laborde", "CT_ObliqueMercator_Rosenmund", "CT_ObliqueMercator_Spherical", "CT_Mercator", "CT_LambertConfConic_2SP", "CT_LambertConfConic_Helmert", "CT_LambertAzimEqualArea", "CT_AlbersEqualArea", "CT_AzimuthalEquidistant", "CT_EquidistantConic", "CT_Stereographic", "CT_PolarStereographic", "CT_ObliqueStereographic", "CT_Equirectangular", "CT_CassiniSoldner", "CT_Gnomonic", "CT_MillerCylindrical", "CT_Orthographic", "CT_Polyconic", "CT_Robinson", "CT_Sinusoidal", "CT_VanDerGrinten", "CT_NewZealandMapGrid", "CT_TransvMercator_SouthOriented"};

                            if (KeyID == 1024)
                            {
                                printf(" - [GTModelTypeGeoKey] : %s\n", GTModelTypeGeoKey_v[Value_Offset]);

                                if (Value_Offset == 2)
                                {
                                    pro.type = proj::ProjType_Equirectangular;
                                }
                            }
                            if (KeyID == 2054)
                            {
                                if (pro.type == proj::ProjType_Equirectangular)
                                {
                                    if (Value_Offset == 9101) // Radians, all good
                                    {
                                        pro.proj_offset_x *= RAD2DEG;
                                        pro.proj_offset_y *= RAD2DEG;
                                        pro.proj_scalar_x *= RAD2DEG;
                                        pro.proj_scalar_y *= RAD2DEG;
                                        printf(" - [GeogAngularUnitsGeoKey] : Radians\n");
                                    }
                                    else if (Value_Offset == 9102)
                                    {
                                        printf(" - [GeogAngularUnitsGeoKey] : Degrees\n");
                                    }
                                    else
                                    {
                                        printf(" - [GeogAngularUnitsGeoKey] : UNSUPPORTED!\n");
                                    }
                                }
                            }
                            else if (KeyID == 3072)
                            {
                                printf(" - [ProjectedCSTypeGeoKey] : %d\n", Value_Offset);

                                if (Value_Offset >= 32601 && Value_Offset <= 32660)
                                {
                                    pro.type = proj::ProjType_UniversalTransverseMercator;
                                    pro.params.zone = Value_Offset - 32600;
                                    pro.params.south = false;
                                }
                                else if (Value_Offset >= 32701 && Value_Offset <= 32760)
                                {
                                    pro.type = proj::ProjType_UniversalTransverseMercator;
                                    pro.params.zone = Value_Offset - 32700;
                                    pro.params.south = true;
                                }
                            }
                            else if (KeyID == 3075)
                            {
                                printf(" - [ProjCoordTransGeoKey] : %s\n", ProjCoordTransGeoKey_v[Value_Offset]);

                                if (Value_Offset == 14)
                                    pro.type = proj::ProjType_Stereographic;
                            }
                            else
                                printf(" - [%d] : %d\n", KeyID, Value_Offset);
                        }
                        else
                        {
                            printf(" - [%d] : NOT_READ %d, %d", KeyID, TIFFTagLocation, Value_Offset);
                            if (KeyID == 3088 || KeyID == 3089)
                            {
                                uint32_t count;
                                double *value;
                                TIFFGetField(tif, TIFFTagLocation, &count, &value);

                                double val = value[Value_Offset];

                                printf(" = %f", val);

                                if (KeyID == 3088)
                                    pro.lam0 = val * DEG2RAD;
                                if (KeyID == 3089)
                                    pro.phi0 = val * DEG2RAD;
                            }
                            printf("\n");
                        }
                    }
                }
            }

            npixels = w * h;
            raster = (uint32_t *)_TIFFmalloc(npixels * sizeof(uint32_t));

            nlohmann::json meta;
            meta["proj_cfg"] = pro;
            set_metadata(*this, meta);

            if (raster != NULL)
            {
                init(w, h, 4);

                if (TIFFReadRGBAImage(tif, w, h, raster, 0))
                {
                    // ... process raster data...

                    for (size_t i = 0; i < npixels; i++)
                    {
                        if (d_depth == 8)
                        {
                            channel(0)[i] = (raster[i] >> 0) & 0xFF;
                            channel(1)[i] = (raster[i] >> 8) & 0xFF;
                            channel(2)[i] = (raster[i] >> 16) & 0xFF;
                            channel(3)[i] = (raster[i] >> 24) & 0xFF;
                        }
                        else if (d_depth == 16)
                        {
                            channel(0)[i] = ((raster[i] >> 0) & 0xFF) << 8;
                            channel(1)[i] = ((raster[i] >> 8) & 0xFF) << 8;
                            channel(2)[i] = ((raster[i] >> 16) & 0xFF) << 8;
                            channel(3)[i] = ((raster[i] >> 24) & 0xFF) << 8;
                        }
                    }
                }
                _TIFFfree(raster);
            }
            TIFFClose(tif);
        }
    }

    // Generate Images for uint16_t and uint8_t
    template class Image<uint8_t>;
    template class Image<uint16_t>;
}

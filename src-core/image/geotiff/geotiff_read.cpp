#include "geotiff.h"
#include "geotiff_def.h"

namespace satdump
{
    namespace geotiff
    {
        bool try_read_geotiff(proj::projection_t *proj, TIFF *tif)
        {
            bool has_tiepoint = false, has_pixelscale = false, has_geokeys = false;
            *proj = proj::projection_t();

            bool debug = true;

            uint32_t geokeys_c;
            uint16_t *geokeys;
            if (TIFFGetField(tif, GEOTIFFTAG_GeoKeyDirectoryTag, &geokeys_c, &geokeys))
            { // Attempt to parse the rest
                if (debug)
                    printf("GeoTIFF Tags :\n");

                // First, check if we have a reference point
                uint32_t tiepoints_c;
                double *tiepoints;
                if (TIFFGetField(tif, GEOTIFFTAG_ModelTiepointTag, &tiepoints_c, &tiepoints))
                {
                    if (tiepoints_c >= 6)
                    {
                        proj->proj_offset_x = tiepoints[3];
                        proj->proj_offset_y = tiepoints[4];
                        has_tiepoint = true;
                    }

                    if (debug)
                    {
                        printf(" - ModelTiepointTag : ");
                        for (uint32_t i = 0; i < tiepoints_c; i++)
                            printf("%f, ", tiepoints[i]);
                        printf("\n");
                    }
                }

                // Secondly, check if have a pixel scale
                // First, check if we have a reference point
                uint32_t pixelscale_c;
                double *pixelscale;
                if (TIFFGetField(tif, GEOTIFFTAG_ModelPixelScaleTag, &pixelscale_c, &pixelscale))
                {
                    if (pixelscale_c >= 2)
                    {
                        proj->proj_scalar_x = pixelscale[0];
                        proj->proj_scalar_y = -pixelscale[1];
                        has_pixelscale = true;
                    }

                    if (debug)
                    {
                        printf(" - ModelPixelScaleTag : ");
                        for (uint32_t i = 0; i < pixelscale_c; i++)
                            printf("%f, ", pixelscale[i]);
                        printf("\n");
                    }
                }

                // Finally, we need to parse the GeoKeys
                if (geokeys_c >= 4)
                {
                    if (debug)
                    {
                        printf(" - GeoKeyDirectoryTag :\n");
                        printf(" --- KeyDirectoryVersion : %d\n", geokeys[0]);
                        printf(" --- KeyRevision : %d\n", geokeys[1]);
                        printf(" --- MinorRevision : %d\n", geokeys[2]);
                        printf(" --- NumberOfKeys : %d\n", geokeys[3]);
                    }

                    for (uint16_t i = 0; i < geokeys[3]; i++)
                    { // We assume KeyIDs are in order!
                        int KeyID = geokeys[4 + i * 4 + 0];
                        int TIFFTagLocation = geokeys[4 + i * 4 + 1];
                        // int Count = geokeys[4 + i * 4 + 2];
                        int Value_Offset = geokeys[4 + i * 4 + 3];

                        if (TIFFTagLocation == 0)
                        { // These are directly in Value_Offset
                            if (KeyID == GTModelTypeGeoKey)
                            { // Model Type
                                if (Value_Offset == ModelTypeGeographic)
                                    proj->type = proj::ProjType_Equirectangular;

                                if (debug)
                                {
                                    const char *GTModelTypeGeoKey_v[] = {"Unknown", "ModelTypeProjected", "ModelTypeGeographic", "ModelTypeGeocentric"};
                                    printf(" --- [GTModelTypeGeoKey] : %s\n", GTModelTypeGeoKey_v[Value_Offset]);
                                }
                            }
                            else if (KeyID == GeogAngularUnitsGeoKey)
                            { // Units. Mainly to switch between degs/rads in equirectangular
                                if (proj->type == proj::ProjType_Equirectangular)
                                {
                                    if (Value_Offset == Angular_Radian) // Radians, all good
                                    {
                                        if (has_pixelscale && has_tiepoint)
                                        {
                                            proj->proj_offset_x *= RAD2DEG;
                                            proj->proj_offset_y *= RAD2DEG;
                                            proj->proj_scalar_x *= RAD2DEG;
                                            proj->proj_scalar_y *= RAD2DEG;
                                        }
                                        if (debug)
                                            printf(" --- [GeogAngularUnitsGeoKey] : Radians\n");
                                    }
                                    else if (Value_Offset == Angular_Degree)
                                    {
                                        if (debug)
                                            printf(" --- [GeogAngularUnitsGeoKey] : Degrees\n");
                                    }
                                    else
                                    {
                                        if (debug)
                                            printf(" --- [GeogAngularUnitsGeoKey] : Unsupported!\n");
                                    }
                                }
                            }
                            else if (KeyID == ProjectedCSTypeGeoKey)
                            { // Projected Model Type. Used to cover UTM and such
                                if (Value_Offset >= PCS_WGS84_UTM_zone_1N && Value_Offset <= PCS_WGS84_UTM_zone_60N)
                                {
                                    proj->type = proj::ProjType_UniversalTransverseMercator;
                                    proj->params.zone = Value_Offset - PCS_WGS84_UTM_zone_1N + 1;
                                    proj->params.south = false;
                                }
                                else if (Value_Offset >= PCS_WGS84_UTM_zone_1S && Value_Offset <= PCS_WGS84_UTM_zone_60S)
                                {
                                    proj->type = proj::ProjType_UniversalTransverseMercator;
                                    proj->params.zone = Value_Offset - PCS_WGS84_UTM_zone_1S + 1;
                                    proj->params.south = true;
                                }
                                else if (Value_Offset == 3857) // WGS 84 / Pseudo-Mercator
                                {
                                    proj->type = proj::ProjType_WebMerc;
                                }

                                if (debug)
                                    printf(" --- [ProjectedCSTypeGeoKey] : %d\n", Value_Offset);
                            }
                            else if (KeyID == ProjCoordTransGeoKey)
                            { // Projection type
                                if (Value_Offset == CT_Stereographic)
                                    proj->type = proj::ProjType_Stereographic;

                                if (debug)
                                {
                                    const char *ProjCoordTransGeoKey_v[] = {"Unknown",
                                                                            "CT_TransverseMercator",
                                                                            "CT_TransvMercator_Modified_Alaska",
                                                                            "CT_ObliqueMercator",
                                                                            "CT_ObliqueMercator_Laborde",
                                                                            "CT_ObliqueMercator_Rosenmund",
                                                                            "CT_ObliqueMercator_Spherical",
                                                                            "CT_Mercator",
                                                                            "CT_LambertConfConic_2SP",
                                                                            "CT_LambertConfConic_Helmert",
                                                                            "CT_LambertAzimEqualArea",
                                                                            "CT_AlbersEqualArea",
                                                                            "CT_AzimuthalEquidistant",
                                                                            "CT_EquidistantConic",
                                                                            "CT_Stereographic",
                                                                            "CT_PolarStereographic",
                                                                            "CT_ObliqueStereographic",
                                                                            "CT_Equirectangular",
                                                                            "CT_CassiniSoldner",
                                                                            "CT_Gnomonic",
                                                                            "CT_MillerCylindrical",
                                                                            "CT_Orthographic",
                                                                            "CT_Polyconic",
                                                                            "CT_Robinson",
                                                                            "CT_Sinusoidal",
                                                                            "CT_VanDerGrinten",
                                                                            "CT_NewZealandMapGrid",
                                                                            "CT_TransvMercator_SouthOriented"};
                                    printf(" --- [ProjCoordTransGeoKey] : %s\n", ProjCoordTransGeoKey_v[Value_Offset]);
                                }
                            }
                        }
                        else
                        { // Stored somewhere else as a TIFF tag!
                            if (KeyID == ProjCenterLongGeoKey || KeyID == ProjCenterLatGeoKey)
                            { // Doubles only
                                uint32_t count;
                                double *value;
                                TIFFGetField(tif, TIFFTagLocation, &count, &value);

                                double val = value[Value_Offset];

                                if (KeyID == ProjCenterLongGeoKey) // Center lon
                                    proj->lam0 = val * DEG2RAD;
                                if (KeyID == ProjCenterLatGeoKey) // Center lat
                                    proj->phi0 = val * DEG2RAD;

                                if (debug)
                                    printf(" --- [%d] : %f\n", KeyID, val);
                            }
                            else
                            {
                                if (debug)
                                    printf(" --- [%d] : NOT_READ\n", KeyID);
                            }
                        }
                    }

                    has_geokeys = true;
                }
            }
            else
            { // GeoTIFF needs GeoKeyDirectoryTag
                return false;
            }

            // Bunch of checks
            if (!has_tiepoint)
            {
                if (debug)
                    printf("No tiepoint!\n");
                return false;
            }
            if (!has_pixelscale)
            {
                if (debug)
                    printf("No pixel scale!\n");
                return false;
            }
            if (!has_geokeys)
            {
                if (debug)
                    printf("No Geokeys!\n");
                return false;
            }
            if (proj->type == proj::ProjType_Invalid)
            {
                if (debug)
                    printf("Projection not set!\n");
                return false;
            }

            if (debug)
                printf("End of GeoTIFF\n");

            // If we get there, it's valid
            return true;
        }
    } // namespace geotiff
} // namespace satdump
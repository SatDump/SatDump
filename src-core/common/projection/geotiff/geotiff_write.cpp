#include "geotiff.h"
#include "geotiff_def.h"
#include "logger.h"

namespace geotiff
{
    bool try_write_geotiff(TIFF *tif, proj::projection_t *proj)
    {
        bool debug = true;

        // Bunch of checks
        if (proj->type == proj::ProjType_Invalid)
        {
            if (debug)
                printf("Projection is invalid!\n");
            return true;
        }

        if (!(proj->type == proj::ProjType_Equirectangular ||
              proj->type == proj::ProjType_Stereographic ||
              proj->type == proj::ProjType_UniversalTransverseMercator))
        {
            if (debug)
                printf("Unsupported projection!\n");
            return true;
        }

        // Register the tags we need
        static const TIFFFieldInfo xtiffFieldInfo[] = {
            //
            {GEOTIFFTAG_ModelPixelScaleTag, -1, -1, TIFF_DOUBLE, FIELD_CUSTOM, true, true, "ModelPixelScaleTag"},
            //  {TIFFTAG_GEOTRANSMATRIX, -1, -1, TIFF_DOUBLE, FIELD_CUSTOM, TRUE, TRUE, "GeoTransformationMatrix"},
            {GEOTIFFTAG_ModelTiepointTag, -1, -1, TIFF_DOUBLE, FIELD_CUSTOM, true, true, "ModelTiepointTag"},
            {GEOTIFFTAG_GeoKeyDirectoryTag, -1, -1, TIFF_SHORT, FIELD_CUSTOM, true, true, "GeoKeyDirectoryTag"},
            {GEOTIFFTAG_GeoKeysDoubles, -1, -1, TIFF_DOUBLE, FIELD_CUSTOM, true, true, "GeoKeysDoubles"},
            //  {TIFFTAG_GEOASCIIPARAMS, -1, -1, TIFF_ASCII, FIELD_CUSTOM, TRUE, FALSE, "GeoASCIIParams"}
            //
        };
#define N(a) (sizeof(a) / sizeof(a[0]))
        TIFFMergeFieldInfo(tif, xtiffFieldInfo, N(xtiffFieldInfo));

        // Start by writing tiepoints
        double tiepoints[6] = {0.0, 0.0, 0.0, proj->proj_offset_x, proj->proj_offset_y, 0.0};
        TIFFSetField(tif, GEOTIFFTAG_ModelTiepointTag, 6, &tiepoints);

        // Then write pixel scales
        double pixelscales[3] = {proj->proj_scalar_x, -proj->proj_scalar_y, 0.0};
        TIFFSetField(tif, GEOTIFFTAG_ModelPixelScaleTag, 3, &pixelscales);

        // Finally, the GeoKeys
        std::vector<uint16_t> geoskeys(4);
        geoskeys[0] = 1; // KeyDirectoryVersion
        geoskeys[1] = 1; // KeyRevision
        geoskeys[2] = 0; // MinorRevision
        geoskeys[3] = 0; // NumberOfKeys

        std::vector<double> doublekeys;
        int keysn = 0;
        {
            // Model Type
            geoskeys.push_back(GTModelTypeGeoKey);
            geoskeys.push_back(0);
            geoskeys.push_back(1);
            if (proj->type == proj::ProjType_Equirectangular)
                geoskeys.push_back(ModelTypeGeographic);
            else
                geoskeys.push_back(ModelTypeProjected);
            keysn++;

            // Projected model type
            if (proj->type == proj::ProjType_UniversalTransverseMercator)
            { // UTM Mercator
                int value = 0;

                printf("ZONE IS %d\n", proj->params.zone);
                if (proj->params.south)
                    value = PCS_WGS84_UTM_zone_1S - 1 + proj->params.zone;
                else
                    value = PCS_WGS84_UTM_zone_1N - 1 + proj->params.zone;

                geoskeys.push_back(ProjectedCSTypeGeoKey);
                geoskeys.push_back(0);
                geoskeys.push_back(1);
                geoskeys.push_back(value);
                keysn++;
            }
            else if (proj->type == proj::ProjType_Stereographic)
            { // Stereographic
                geoskeys.push_back(ProjCoordTransGeoKey);
                geoskeys.push_back(0);
                geoskeys.push_back(1);
                geoskeys.push_back(CT_Stereographic);
                keysn++;
            }
            else if (proj->type == proj::ProjType_WebMerc)
            {
                geoskeys.push_back(ProjectedCSTypeGeoKey);
                geoskeys.push_back(0);
                geoskeys.push_back(1);
                geoskeys.push_back(3857);
                keysn++;
            }

            // Longitude/Latitude of origin
            if (proj->lam0 != 0)
            { // Longitude is not 0
                doublekeys.push_back(proj->lam0 * RAD2DEG);
                geoskeys.push_back(ProjCenterLongGeoKey);
                geoskeys.push_back(GEOTIFFTAG_GeoKeysDoubles);
                geoskeys.push_back(1);
                geoskeys.push_back(doublekeys.size() - 1);
                keysn++;
            }
            if (proj->phi0 != 0)
            { // Latitude is not 0
                doublekeys.push_back(proj->phi0 * RAD2DEG);
                geoskeys.push_back(ProjCenterLatGeoKey);
                geoskeys.push_back(GEOTIFFTAG_GeoKeysDoubles);
                geoskeys.push_back(1);
                geoskeys.push_back(doublekeys.size() - 1);
                keysn++;
            }

            // Angular unit. Always degree for us
            if (proj->type == proj::ProjType_Equirectangular ||
                proj->lam0 != 0 ||
                proj->phi0 != 0)
            {
                geoskeys.push_back(GeogAngularUnitsGeoKey);
                geoskeys.push_back(0);
                geoskeys.push_back(1);
                geoskeys.push_back(Angular_Degree);
                keysn++;
            }

            // Ellipsoid model. Always WGS84 for us!
            {
                geoskeys.push_back(GeographicTypeGeoKey);
                geoskeys.push_back(0);
                geoskeys.push_back(1);
                geoskeys.push_back(GCS_WGS_84);
                keysn++;
            }
        }

        geoskeys[3] = keysn;

        uint16_t *geokeys_ptr = geoskeys.data();
        TIFFSetField(tif, GEOTIFFTAG_GeoKeyDirectoryTag, geoskeys.size(), geokeys_ptr);

        if (doublekeys.size() > 0)
        {
            double *doublekeys_ptr = doublekeys.data();
            TIFFSetField(tif, GEOTIFFTAG_GeoKeysDoubles, doublekeys.size(), doublekeys_ptr);
        }

        return false;
    }
}
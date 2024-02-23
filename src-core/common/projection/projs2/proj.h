#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#define M_HALFPI M_PI_2
#define M_TWOPI (M_PI * 2)

#define DEG2RAD ((2.0 * M_PI) / 360.0)
#define RAD2DEG (360.0 / (2.0 * M_PI))

#include <cstdio>

namespace proj
{
    enum projection_type_t
    {
        ProjType_Invalid,
        ProjType_Equirectangular,
        ProjType_Stereographic,
        ProjType_UniversalTransverseMercator,
        ProjType_Geos,
    };

    struct projection_setup_t
    {
        int zone = -1;        // For UTM
        bool south = false;   // For UTM
        bool sweep_x = false; // For GEOS
        double altitude = 0;  // For GEOS
    };

    struct projection_t
    {
        // Core
        projection_type_t type = ProjType_Invalid; // Projection type
        projection_setup_t params;                 // Other setup parameters
        void *proj_dat = nullptr;                  // Opaque pointer holding projection-specific info

        // Offsets & Scalars.
        double proj_offset_x = 0; // False Easting
        double proj_offset_y = 0; // False Northing
        double proj_scalar_x = 1; // X Scalar
        double proj_scalar_y = 1; // Y Scalar

        // Internal Offsets & Scalars
        double lam0 = 0; // Central Meridian
        double phi0 = 0; // Central Parrallel
        double k0 = 1;   // General scaling factor - e.g. the 0.9996 of UTM
        double x0 = 0;   // False Easting
        double y0 = 0;   // False Northing

        // Ellispoid definition
        double a;
        double e;
        double es;
        double n;
        double one_es;
        double rone_es;
    };

    /*
    Setup a projection. This expects the struct to already
    be pre-configured by the user and takes care of
    allocations and setting the ellispoid.
    */
    bool projection_setup(projection_t *proj);

    /*
    Free allocated memory, by projection_setup.
    */
    void projection_free(projection_t *proj);

    /*
    Perform a forward projection.
    This converts Lat/Lon into X/Y in the projected plane.
    Lat/Lon should be in degrees.
    Returns true if an error occurred, false on success.
    */
    bool projection_perform_fwd(projection_t *proj, double lon, double lat, double *x, double *y);

    /*
    Perform an inverse projection.
    This converts X/Y from the projected plane into Lat/Lon.
    Lat/Lon will be in degrees.
    Returns true if an error occurred, false on success.
    */
    bool projection_perform_inv(projection_t *proj, double x, double y, double *lon, double *lat);
}
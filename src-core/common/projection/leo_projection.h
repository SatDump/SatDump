#pragma once

#include <vector>
#include "tle.h"
#include "tpers.h"
#include "common/predict/predict.h"

/*
Code to reference a decoded image (or similar data) from a LEO satellite to Lat / Lon coordinates.
It works by calculating the satellite's position at the time of each scan, and generating a projection 
for each of them. That projection references any point visible from the satellite's position to a
known Lat / Lon.
Doing this for every scan, with some curvature correction to match the image to the projection allows
referencing any given point.
A look-up-table is generated in the constructor to speed up later processing.

You may also notice there is no forward function (eg, Lat / Lon to x/y on the image). That's because
the easiest way to do it would be getting the closest point in the generated LUT, but doing this
efficiently gets complicated quickly...

PS : I am not sure in any way that this is a good implementation, there may be very obvious mistakes in there...

PS #2 : Currently the projection is aligned to the satellite's vector by computing previous and later positions
of the satellite. It's approximate and may cause issues later but I have not found a proper way to get velocity
vectors relative to the satellite itself from the ECI values. At least, not in a form I can use for this purpose.
*/
namespace projection
{
    struct LEOScanProjectorSettings
    {
        const double proj_offset;                 // Projection pixel offset, horizontal
        const int correction_swath;               // Curvature correction to match the titled perspective projection. Not real instrument swath in most cases
        const double correction_res;              // Instrument resolution for curvature correction
        const float correction_height;            // Satellite height for curvature correction. Probably what to tune in most cases
        const double instrument_swath;            // For coverage computation. The instrument scans at an angle relative to the entire satellite FOV
        const double proj_scale;                  // Projection scalling compared to the full satellite footprint
        const double az_offset;                   // Azimuth offset, if the instrument is angled relative the sat's motion vector
        const double tilt_offset;                 // Instrument tilt if it's not pointing at NADIR. Usually 0
        const double time_offset;                 // Timestamp offset relative to the provided timestamps
        const int image_width;                    // Input image width
        const bool invert_scan;                   // Invert the scan direction relative to the projection
        const tle::TLE sat_tle;                   // Satellite TLEs
        const std::vector<double> utc_timestamps; // Timestamps. Must match each scanline of the image you will be working with
    };

    class LEOScanProjector
    {
    private:
        // Settings
        const LEOScanProjectorSettings settings;

        // Luts and values used for referencing each line
        std::vector<projection::TPERSProjection> projs;
        std::vector<double> sat_footprints;
        std::vector<float> curvature_correction_factors_fwd; // Corrected input, original image output
        std::vector<float> curvature_correction_factors_inv; // Uncorrected input, corrected output
        int corrected_width;

        // Global lat / lon LUT
        std::vector<std::vector<std::pair<double, double>>> latlon_lut;

        // Internal functions
        void initCurvatureTable(); // Init curvature correction table
        void generateProjections();

    public:
        std::vector<predict_position> poss;
        LEOScanProjector(LEOScanProjectorSettings settings);

        int inverse(int img_x, int img_y, double &lat, double &lon, bool correct = true); // Transform image coordinates to lat / lon. Return 1 if there was an error
        //void inverse(int img_x, int img_y, double &lat, double &lon);                       // Transform image coordinates to lat / lon. Calls up a LUT to be faster
    };
};
#pragma once

#include <vector>
#include "tle.h"
#include "common/geodetic/geodetic_coordinates.h"
#include "libs/predict/predict.h"

/*
Code to reference a decoded image (or similar data) from a LEO satellite to Lat / Lon coordinates.
It works by calculating the satellite's position at the time of each scan, and generating a projection 
for each of them. That projection references any point visible from the satellite's position to a
known Lat / Lon.
Doing this for every scan, with some curvature correction to match the image to the projection allows
referencing any given point.
A look-up-table is generated in the constructor to speed up later processing.

You may also notice there is no forward function (eg, Lat / Lon to x/y on the image). That's because
the easiest way to do it would be getting the closest point in a generated LUT, but doing this
efficiently gets complicated quickly...

PS : I am not sure in any way that this is a good implementation, there may be very obvious mistakes in there...

PS #2 : Currently the projection is aligned to the satellite's vector by computing previous and later positions
of the satellite. It's approximate and may cause issues later but I have not found a proper way to get velocity
vectors relative to the satellite itself from the ECI values. At least, not in a form I can use for this purpose.

PS #3 : Currently this only handles a single timestamp per scanline, which is usually most instruments work. However,
some don't like MODIS, IASI, etc. They instead send a timestamp per "IFOV" (Group of samples), such as 64x64 at once
for IASI. Others like MERSI send a full scanline of 40 pixels at once... It would be good to handle those cases properly,
utilizing each individual timestamp and properly simulating the group of detector etc...
I guess this calls for writing variants of this code later on.
*/
namespace geodetic
{
    namespace projection
    {
        struct LEOScanProjectorSettings
        {
            double scan_angle;                  // Total scan angle
            double roll_offset;                 // Roll offset
            double pitch_offset;                // Pitch offset
            double yaw_offset;                  // Yaw offset
            double time_offset;                 // Timestamp offset relative to the provided timestamps
            int image_width;                    // Input image width
            bool invert_scan;                   // Invert the scan direction relative to the projection
            tle::TLE sat_tle;                   // Satellite TLEs
            std::vector<double> utc_timestamps; // Timestamps. Must match each scanline of the image you will be working with
        };

        class LEOScanProjector
        {
        private:
            // Settings
            const LEOScanProjectorSettings settings;

            // Luts and values used for referencing each line
            std::vector<geodetic_coords_t> satellite_positions;
            std::vector<double> satellite_directions;
            std::vector<bool> satellite_is_asc;

            // Internal functions
            void generateOrbit();

        public:
            std::vector<predict_position> poss;
            LEOScanProjector(LEOScanProjectorSettings settings);

            int inverse(int img_x, int img_y, geodetic_coords_t &coords); // Transform image coordinates to lat / lon. Return 1 if there was an error
            //void inverse(int img_x, int img_y, double &lat, double &lon);                       // Transform image coordinates to lat / lon. Calls up a LUT to be faster
        };
    };
};
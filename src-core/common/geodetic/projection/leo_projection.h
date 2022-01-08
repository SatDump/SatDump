#pragma once

#include <vector>
#include "tle.h"
#include <memory>
#include "common/geodetic/geodetic_coordinates.h"
#include "libs/predict/predict.h"
#include "tps_transform.h"
#include <mutex>

/*
Code to reference a decoded image (or similar data) from a LEO satellite to Lat / Lon coordinates.
It works by calculating the satellite's position at the time of each scan, and generating a projection 
for each of them. That projection references any point visible from the satellite's position to a
known Lat / Lon.
Doing this for every scan, with some curvature correction to match the image to the projection allows
referencing any given point.

The forward function is implemented using a Thin Plate Transform (from GDAL), which is calculated using
a few reference points.
By default this TPS projection is NOT setup, and only will upon calling setup_forward(). This is because 
resolving the TPS equations is rather slow, and in most cases is not required... Hence this is left to the user
to call whenever a forward transform is required.

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
        enum LEOProjectionType
        {
            TIMESTAMP_PER_SCANLINE = 0,
            TIMESTAMP_PER_IFOV = 1,
        };

        struct LEOScanProjectorSettings
        {
            LEOProjectionType type; // Projection type
        };

        struct LEOScanProjectorSettings_SCANLINE : public LEOScanProjectorSettings
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

            LEOScanProjectorSettings_SCANLINE(double scan_angle,
                                              double roll_offset,
                                              double pitch_offset,
                                              double yaw_offset,
                                              double time_offset,
                                              int image_width,
                                              bool invert_scan,
                                              tle::TLE sat_tle,
                                              std::vector<double> utc_timestamps)
                : scan_angle(scan_angle),
                  roll_offset(roll_offset),
                  pitch_offset(pitch_offset),
                  yaw_offset(yaw_offset),
                  time_offset(time_offset),
                  image_width(image_width),
                  invert_scan(invert_scan),
                  sat_tle(sat_tle),
                  utc_timestamps(utc_timestamps)
            {
                type = TIMESTAMP_PER_SCANLINE;
            }
        };

        std::shared_ptr<LEOScanProjectorSettings_SCANLINE> makeScalineSettingsFromJSON(std::string filename);

        struct LEOScanProjectorSettings_IFOV : public LEOScanProjectorSettings
        {
            double scan_angle;                               // Total scan angle
            double ifov_x_scan_angle;                        // IFOV X scan angle
            double ifov_y_scan_angle;                        // IFOV Y scan angle
            double roll_offset;                              // Roll offset
            double pitch_offset;                             // Pitch offset
            double yaw_offset;                               // Yaw offset
            double time_offset;                              // Timestamp offset relative to the provided timestamps
            int ifov_count;                                  // Number of IFOVs
            int ifov_x_size;                                 // IFOV Width
            int ifov_y_size;                                 // IFOV Height
            int image_width;                                 // Input image width
            bool invert_scan;                                // Invert the scan direction relative to the projection
            tle::TLE sat_tle;                                // Satellite TLEs
            std::vector<std::vector<double>> utc_timestamps; // Timestamps. Must match each scanline of the image you will be working with

            LEOScanProjectorSettings_IFOV(double scan_angle,
                                          double ifov_x_scan_angle,
                                          double ifov_y_scan_angle,
                                          double roll_offset,
                                          double pitch_offset,
                                          double yaw_offset,
                                          double time_offset,
                                          int ifov_count,
                                          int ifov_x_size,
                                          int ifov_y_size,
                                          int image_width,
                                          bool invert_scan,
                                          tle::TLE sat_tle,
                                          std::vector<std::vector<double>> utc_timestamps)
                : scan_angle(scan_angle),
                  ifov_x_scan_angle(ifov_x_scan_angle),
                  ifov_y_scan_angle(ifov_y_scan_angle),
                  roll_offset(roll_offset),
                  pitch_offset(pitch_offset),
                  yaw_offset(yaw_offset),
                  time_offset(time_offset),
                  ifov_count(ifov_count),
                  ifov_x_size(ifov_x_size),
                  ifov_y_size(ifov_y_size),
                  image_width(image_width),
                  invert_scan(invert_scan),
                  sat_tle(sat_tle),
                  utc_timestamps(utc_timestamps) { type = TIMESTAMP_PER_IFOV; }
        };

        class LEOScanProjector
        {
        private:
            // Settings
            const std::shared_ptr<LEOScanProjectorSettings> settings;

            // Luts and values used for referencing each line
            std::vector<geodetic_coords_t> satellite_positions;
            std::vector<double> satellite_directions;
            std::vector<bool> satellite_is_asc;

            // Internal functions
            void generateOrbit_SCANLINE();
            void generateOrbit_IFOV();

            // Forward transform
            TPSTransform tps;
            bool forward_ready = false;
            int img_height;
            int img_width;

            std::mutex solvingMutex; // To make it thread-safe

        public:
            std::vector<predict_position> poss;
            LEOScanProjector(std::shared_ptr<LEOScanProjectorSettings> settings);
            std::shared_ptr<LEOScanProjectorSettings> getSettings();

            int inverse(int img_x, int img_y, geodetic_coords_t &coords);                                                        // Transform image coordinates to lat / lon. Return 1 if there was an error
            int forward(geodetic_coords_t coords, int &img_x, int &img_y);                                                       // Transform lat / lon coordinates to image coordinates
            int setup_forward(float timestamp_max = 99.0f, float timestamp_mix = 1.0f, int gcp_lines = 10, int gcp_px_cnt = 10); // Setup forward projection. This can be slow for larger images!
        };
    };
};
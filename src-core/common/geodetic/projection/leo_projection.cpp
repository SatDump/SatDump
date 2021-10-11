#include "leo_projection.h"

#include <cmath>
#ifndef M_PI
#define M_PI (3.14159265359)
#endif
#include "libs/predict/predict.h"
#include "logger.h"
#include "common/geodetic/euler_raytrace.h"
#include "common/geodetic/vincentys_calculations.h"

namespace geodetic
{
    namespace projection
    {
        void LEOScanProjector::generateOrbit()
        {
            // Setup SGP4 model
            predict_orbital_elements_t *satellite_object = predict_parse_tle(settings.sat_tle.line1.c_str(),
                                                                             settings.sat_tle.line2.c_str());
            predict_position satellite_orbit;
            predict_position satellite_pos2;

            for (int currentScan = 0; currentScan < (int)settings.utc_timestamps.size(); currentScan++)
            {
                double currentTimestamp = settings.utc_timestamps[currentScan] + settings.time_offset;

                // Get Julian time of the scan, with full accuracy and calculate the satellite's
                // position at the time
                predict_julian_date_t currentJulianTime = predict_to_julian_double(currentTimestamp);
                predict_orbit(satellite_object, &satellite_orbit, currentJulianTime);
                satellite_positions.push_back(geodetic::geodetic_coords_t(satellite_orbit.latitude, satellite_orbit.longitude, satellite_orbit.altitude, true));

                // Calculate Az to use from the satellite's motion vector
                predict_orbit(satellite_object, &satellite_pos2, predict_to_julian_double(currentTimestamp - 1));

                double az_angle_vincentis = vincentys_inverse(geodetic_coords_t(satellite_pos2.latitude, satellite_pos2.longitude, satellite_pos2.altitude, true),
                                                              geodetic_coords_t(satellite_orbit.latitude, satellite_orbit.longitude, satellite_orbit.altitude, true))
                                                .reverse_azimuth;
                az_angle_vincentis -= M_PI;

                satellite_directions.push_back(az_angle_vincentis * RAD_TO_DEG);
                satellite_is_asc.push_back(satellite_pos2.latitude < satellite_orbit.latitude);
            }
        }

        LEOScanProjector::LEOScanProjector(LEOScanProjectorSettings settings) : settings(settings)
        {
            logger->info("Compute orbit...");
            generateOrbit();
        }

        int LEOScanProjector::inverse(int img_x, int img_y, geodetic_coords_t &ground_position)
        {
            // Check we're in bounds
            if (img_y > (int)satellite_positions.size() || img_x >= settings.image_width)
                return 1;

            double final_x = settings.invert_scan ? (settings.image_width - 1) - img_x : img_x;

            geodetic::geodetic_coords_t &satellite_position = satellite_positions[img_y];

            const bool &ascending = satellite_is_asc[img_y]; // Not a fan of this, but it works OK and doesn't seem too bad so well, whatever I guess?

            geodetic::euler_coords_t satellite_pointing;
            satellite_pointing.roll = -(((final_x - (settings.image_width / 2)) / settings.image_width) * settings.scan_angle) + settings.roll_offset;
            satellite_pointing.pitch = settings.pitch_offset;
            satellite_pointing.yaw = (90 + (ascending ? settings.yaw_offset : -settings.yaw_offset)) - satellite_directions[img_y];

            geodetic::raytrace_to_earth(satellite_position, satellite_pointing, ground_position);
            ground_position.toDegs();

            return 0;
        }
    };
};
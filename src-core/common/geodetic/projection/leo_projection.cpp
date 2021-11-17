#include "leo_projection.h"

#include <cmath>
#ifndef M_PI
#define M_PI (3.14159265359)
#endif
#include "libs/predict/predict.h"
#include "logger.h"
#include "common/geodetic/euler_raytrace.h"
#include "common/geodetic/vincentys_calculations.h"
#include "common/utils.h"

namespace geodetic
{
    namespace projection
    {
        void LEOScanProjector::generateOrbit_SCANLINE()
        {
            LEOScanProjectorSettings_SCANLINE *settings = (LEOScanProjectorSettings_SCANLINE *)this->settings.get();
            img_height = settings->utc_timestamps.size();
            img_width = settings->image_width;

            // Setup SGP4 model
            predict_orbital_elements_t *satellite_object = predict_parse_tle(settings->sat_tle.line1.c_str(),
                                                                             settings->sat_tle.line2.c_str());
            predict_position satellite_orbit;
            predict_position satellite_pos2;

            for (int currentScan = 0; currentScan < img_height; currentScan++)
            {
                double currentTimestamp = settings->utc_timestamps[currentScan] + settings->time_offset;

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

        void LEOScanProjector::generateOrbit_IFOV()
        {
            LEOScanProjectorSettings_IFOV *settings = (LEOScanProjectorSettings_IFOV *)this->settings.get();
            img_height = settings->utc_timestamps.size() * settings->ifov_y_size;
            img_width = settings->image_width;

            // Setup SGP4 model
            predict_orbital_elements_t *satellite_object = predict_parse_tle(settings->sat_tle.line1.c_str(),
                                                                             settings->sat_tle.line2.c_str());
            predict_position satellite_orbit;
            predict_position satellite_pos2;

            for (int currentScan = 0; currentScan < (int)settings->utc_timestamps.size(); currentScan++)
            {
                for (int currentIfov = 0; currentIfov < settings->ifov_count; currentIfov++)
                {
                    double currentTimestamp = settings->utc_timestamps[currentScan][currentIfov] + settings->time_offset;

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
        }

        LEOScanProjector::LEOScanProjector(std::shared_ptr<LEOScanProjectorSettings> settings) : settings(settings)
        {
            logger->info("Compute orbit...");
            if (settings->type == TIMESTAMP_PER_SCANLINE)
                generateOrbit_SCANLINE();
            else if (settings->type == TIMESTAMP_PER_IFOV)
                generateOrbit_IFOV();
        }

        std::shared_ptr<LEOScanProjectorSettings> LEOScanProjector::getSettings()
        {
            return settings;
        }

        int LEOScanProjector::inverse(int img_x, int img_y, geodetic_coords_t &ground_position)
        {
            if (settings->type == TIMESTAMP_PER_SCANLINE)
            {
                LEOScanProjectorSettings_SCANLINE *settings = (LEOScanProjectorSettings_SCANLINE *)this->settings.get();

                // Check we're in bounds
                if (img_y > (int)satellite_positions.size() || img_x >= settings->image_width)
                    return 1;

                if (settings->utc_timestamps[img_y] == -1)
                    return 1; // If we hit this => no timestamp

                double final_x = settings->invert_scan ? (settings->image_width - 1) - img_x : img_x;

                geodetic::geodetic_coords_t &satellite_position = satellite_positions[img_y];

                const bool &ascending = satellite_is_asc[img_y]; // Not a fan of this, but it works OK and doesn't seem too bad so well, whatever I guess?

                geodetic::euler_coords_t satellite_pointing;
                satellite_pointing.roll = -(((final_x - (settings->image_width / 2)) / settings->image_width) * settings->scan_angle) + settings->roll_offset;
                satellite_pointing.pitch = settings->pitch_offset;
                satellite_pointing.yaw = (90 + (ascending ? settings->yaw_offset : -settings->yaw_offset)) - satellite_directions[img_y];

                geodetic::raytrace_to_earth(satellite_position, satellite_pointing, ground_position);
                ground_position.toDegs();

                return 0;
            }
            else if (settings->type == TIMESTAMP_PER_IFOV)
            {
                LEOScanProjectorSettings_IFOV *settings = (LEOScanProjectorSettings_IFOV *)this->settings.get();

                // Check we're in bounds
                if (img_y > int(settings->utc_timestamps.size() * settings->ifov_y_size) || img_x >= settings->image_width)
                    return 1;

                double final_x = settings->invert_scan ? (settings->image_width - 1) - img_x : img_x;

                int currentScan = img_y / settings->ifov_y_size;
                int currentIfov = final_x / settings->ifov_x_size;
                int currentArrayValue = currentScan * settings->ifov_count + currentIfov;

                if (settings->utc_timestamps[currentScan][currentIfov] == -1)
                    return 1; // If we hit this => no timestamp

                geodetic::geodetic_coords_t &satellite_position = satellite_positions[currentArrayValue];

                const bool &ascending = satellite_is_asc[currentArrayValue]; // Not a fan of this, but it works OK and doesn't seem too bad so well, whatever I guess?

                double currentIfovOffset = -(((double(currentIfov) - (double(settings->ifov_count) / 2)) / double(settings->ifov_count)) * settings->scan_angle);
                double ifov_x = int(final_x) % settings->ifov_x_size;
                double ifov_y = (settings->ifov_y_size - 1) - (img_y % settings->ifov_y_size);

                //logger->info(currentIfovOffset);

                geodetic::euler_coords_t satellite_pointing;
                satellite_pointing.roll = -(((ifov_x - (settings->ifov_x_size / 2)) / settings->ifov_x_size) * settings->ifov_x_scan_angle) + currentIfovOffset + settings->roll_offset;
                satellite_pointing.pitch = -(((ifov_y - (settings->ifov_y_size / 2)) / settings->ifov_y_size) * settings->ifov_y_scan_angle) + settings->pitch_offset;
                satellite_pointing.yaw = (90 + (ascending ? settings->yaw_offset : -settings->yaw_offset)) - satellite_directions[currentArrayValue];

                geodetic::raytrace_to_earth(satellite_position, satellite_pointing, ground_position);
                ground_position.toDegs();

                return 0;
            }
            else
            {
                return 1;
            }
        }

        int LEOScanProjector::forward(geodetic_coords_t coords, int &img_x, int &img_y)
        {
            if (!forward_ready)
                return 1;

            double x;
            double y;
            tps.forward(coords.lon, coords.lat, x, y);

            if (x < 0 || x > img_width)
                return 1;
            if (y < 0 || y > img_height)
                return 1;

            img_x = x;
            img_y = y;

            return 0;
        }

        int LEOScanProjector::setup_forward(float timestamp_max, float timestamp_mix, int gcp_lines, int gcp_px_cnt)
        {
            solvingMutex.lock();

            if (forward_ready)
                return 0;

            logger->info("Setting up forward projection...");

            std::vector<geodetic::projection::GCP> gcps;

            if (settings->type == TIMESTAMP_PER_SCANLINE)
            {
                // We'll create another projector so... Copy settings over
                geodetic::projection::LEOScanProjectorSettings_SCANLINE lsettings = *(LEOScanProjectorSettings_SCANLINE *)this->settings.get();

                // Filter timestamps, allows to cleanup the projection a lot.
                {
                    std::vector<double> sorted_timestamps = lsettings.utc_timestamps;
                    std::sort(sorted_timestamps.begin(), sorted_timestamps.end());

                    double max = percentile<double>(&sorted_timestamps[0], sorted_timestamps.size(), timestamp_max);
                    double min = percentile<double>(&sorted_timestamps[0], sorted_timestamps.size(), timestamp_mix);

                    for (double &ctTimestamp : lsettings.utc_timestamps)
                    {
                        if (ctTimestamp > max || ctTimestamp < min)
                        {
                            ctTimestamp = -1; // Disable those that are out of the filtered range
                        }
                    }
                }

                // Generate Ground Control Points
                std::shared_ptr<geodetic::projection::LEOScanProjectorSettings_SCANLINE>
                    psettings(new geodetic::projection::LEOScanProjectorSettings_SCANLINE(lsettings),
                              [](geodetic::projection::LEOScanProjectorSettings_SCANLINE *) {}); // Do not delete
                geodetic::projection::LEOScanProjector projector(psettings);

                geodetic::geodetic_coords_t latlon;
                for (double y = 1; y < img_height - 1;)
                {
                    geodetic::geodetic_coords_t latlonOld, latlonCurrent, latLonNext;
                    projector.inverse(lsettings.image_width / 2, y - 1, latlonOld);
                    projector.inverse(lsettings.image_width / 2, y, latlonCurrent);
                    projector.inverse(lsettings.image_width / 2, y + 1, latLonNext);

                    // Ensure the current line is between the preceding and next one.
                    // This filters a LOT of bad data out
                    if ((latlonOld.lat < latlonCurrent.lat && latlonCurrent.lat < latLonNext.lat) ||
                        (latlonOld.lat > latlonCurrent.lat && latlonCurrent.lat > latLonNext.lat))
                    {
                        for (double x = 0; x < lsettings.image_width + 1; x += int(lsettings.image_width / double(gcp_px_cnt)))
                        {
                            if (!projector.inverse(x, y, latlon))
                                gcps.push_back(geodetic::projection::GCP{x, y, latlon.lon, latlon.lat});
                        }

                        y += gcp_lines;
                    }
                    else
                    {
                        y++;
                    }
                }
            }
            else
            {
                solvingMutex.unlock();
                return 1;
            }

            logger->info(std::to_string(gcps.size()) + " GCPs");

            // Init TPS transform
            if (tps.init(gcps))
            {
                logger->error("Error setting up forward transform!");
                solvingMutex.unlock();
                return 1;
            }

            forward_ready = true;

            solvingMutex.unlock();

            return 0;
        }
    };
};
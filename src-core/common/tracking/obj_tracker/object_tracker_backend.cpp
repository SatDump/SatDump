#include "object_tracker.h"
#include "common/geodetic/geodetic_coordinates.h"
#include "common/utils.h"
#include "logger.h"
#include "common/tracking/tle.h"
#include <cfloat>

namespace satdump
{
    void ObjectTracker::backend_run()
    {
        while (backend_should_run)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            if (!has_tle)
                continue;

            general_mutex.lock();

            double current_time = getTime();

            if (tracking_mode == TRACKING_HORIZONS)
            {
                if (current_time > last_horizons_fetch_time + 3600)
                {
                    loadHorizons(current_time);
                    updateNextPass(current_time);
                    backend_needs_update = false;
                }

                if (horizons_data.size() > 0)
                {
                    //    size_t iter = 0;
                    //    for (size_t i = 0; i < horizons_data.size(); i++)
                    //        if (horizons_data[i].timestamp < current_time)
                    //            iter = i;

                    if (current_time > next_los_time)
                        updateNextPass(current_time);

                    horizons_interpolate(current_time, &sat_current_pos.az, &sat_current_pos.el);
                }
            }
            else if (tracking_mode == TRACKING_SATELLITE)
            {
                if (satellite_object != nullptr)
                {
                    predict_orbit(satellite_object, &satellite_orbit, predict_to_julian_double(current_time));
                    predict_observe_orbit(satellite_observer_station, &satellite_orbit, &satellite_observation_pos);

                    if (current_time > next_los_time)
                        updateNextPass(current_time);

                    sat_current_pos.az = satellite_observation_pos.azimuth * RAD_TO_DEG;
                    sat_current_pos.el = satellite_observation_pos.elevation * RAD_TO_DEG;
                }
            }

            // Update
            if (backend_needs_update)
            {
                logger->trace("Updating elements...");

                if (tracking_mode == TRACKING_HORIZONS)
                {
                    loadHorizons(current_time);
                    updateNextPass(current_time);
                }
                else if (tracking_mode == TRACKING_SATELLITE)
                {
                    if (satellite_object != nullptr)
                        predict_destroy_orbital_elements(satellite_object);

                    auto &tle = general_tle_registry[current_satellite_id];

                    satellite_object = predict_parse_tle(tle.line1.c_str(), tle.line2.c_str());
                    updateNextPass(current_time);
                }

                backend_needs_update = false;
            }

            general_mutex.unlock();
        }
    }

    void ObjectTracker::updateNextPass(double current_time)
    {
        upcoming_passes_mtx.lock();
        logger->trace("Update pass trajectory...");

        upcoming_pass_points.clear();
        next_aos_time = 0;
        next_los_time = 0;
        northbound_cross = false;
        southbound_cross = false;

        if (tracking_mode == TRACKING_HORIZONS)
        {
            if (horizons_data.size() == 0)
            {
                upcoming_passes_mtx.unlock();
                return;
            }

            int iter = 0;
            for (int i = 0; i < (int)horizons_data.size(); i++)
                if (horizons_data[i].timestamp < current_time)
                    iter = i;

            if (horizons_data[iter].el > 0) // Already got AOS
            {
                next_aos_time = current_time;

                for (int i = iter - 1; i >= 0; i--) // Attempt to find previous AOS
                {
                    if (horizons_data[i].el <= 0)
                    {
                        next_aos_time = horizons_data[i].timestamp;
                        sat_next_aos_pos.az = horizons_data[i].az;
                        sat_next_aos_pos.el = horizons_data[i].el;
                        break;
                    }
                }

                for (int i = iter; i < (int)horizons_data.size(); i++) // Find LOS
                {
                    if (horizons_data[i].el <= 0)
                    {
                        next_los_time = horizons_data[i].timestamp;
                        break;
                    }
                }
            }
            else
            {
                int aos_iter = 0;
                for (int i = iter; i < (int)horizons_data.size(); i++) // Find AOS
                {
                    if (horizons_data[i].el > 0)
                    {
                        next_aos_time = horizons_data[i].timestamp;
                        sat_next_aos_pos.az = horizons_data[i].az;
                        sat_next_aos_pos.el = horizons_data[i].el;
                        aos_iter = i;
                        break;
                    }
                }

                if (next_aos_time != 0)
                {
                    for (int i = aos_iter; i < (int)horizons_data.size(); i++) // Find LOS
                    {
                        if (horizons_data[i].el <= 0)
                        {
                            next_los_time = horizons_data[i].timestamp;
                            break;
                        }
                    }
                }
            }

            if (/*is_gui &&*/ next_aos_time != 0 && next_los_time != 0)
            {
                double time_step = abs(next_los_time - next_aos_time) / 50.0;

                for (double ctime = next_aos_time; ctime <= next_los_time; ctime += time_step)
                {
                    int iter = 0;
                    for (int i = 0; i < (int)horizons_data.size(); i++)
                        if (horizons_data[i].timestamp < ctime)
                            iter = i;

                    upcoming_pass_points.push_back({horizons_data[iter].az, horizons_data[iter].el});
                }
            }
        }
        else if (tracking_mode == TRACKING_SATELLITE)
        {
            if (predict_is_geosynchronous(satellite_object))
            {
                next_aos_time = 0;
                next_los_time = DBL_MAX;
                upcoming_passes_mtx.unlock();
                return;
            }

            // Get next LOS
            predict_observation next_aos, next_los;
            next_aos = next_los = predict_next_los(satellite_observer_station, satellite_object, predict_to_julian_double(getTime()));

            // Calculate the AOS before that LOS
            next_aos_time = next_los_time = predict_from_julian(next_los.time);
            do
            {
                next_aos = predict_next_aos(satellite_observer_station, satellite_object, predict_to_julian_double(next_aos_time));
                next_aos_time -= 10;
            } while (predict_from_julian(next_aos.time) >= next_los_time);

            next_los_time = predict_from_julian(next_los.time);
            next_aos_time = predict_from_julian(next_aos.time);

            sat_next_aos_pos.az = next_aos.azimuth * RAD_TO_DEG;
            sat_next_aos_pos.el = next_aos.elevation * RAD_TO_DEG;
            sat_next_los_pos.az = next_los.azimuth * RAD_TO_DEG;
            sat_next_los_pos.el = next_los.elevation * RAD_TO_DEG;

            if (meridian_flip_correction)
            {
                // Determine pass direction
                predict_position satellite_orbit2;
                predict_observation observation_pos_cur;
                predict_observation observation_pos_prev;
                double currTime = next_aos_time;
                predict_orbit(satellite_object, &satellite_orbit2, predict_to_julian_double(currTime));
                predict_observe_orbit(satellite_observer_station, &satellite_orbit2, &observation_pos_prev);
                currTime += 1.0;
                do
                {
                    predict_orbit(satellite_object, &satellite_orbit2, predict_to_julian_double(currTime));
                    predict_observe_orbit(satellite_observer_station, &satellite_orbit2, &observation_pos_cur);

                    if (std::abs(observation_pos_prev.azimuth - observation_pos_cur.azimuth) > (180 * DEG_TO_RAD))
                    {
                        if (next_los.azimuth > (90 * DEG_TO_RAD) && next_los.azimuth < (270 * DEG_TO_RAD))
                            southbound_cross = true;
                        else
                            northbound_cross = true;
                    }
                    currTime += 1.0;
                    observation_pos_prev = observation_pos_cur;
                } while (next_los_time > currTime);
            }

            if (true) //(is_gui)
            {
                // Calculate a few points during the pass
                predict_position satellite_orbit2;
                predict_observation observation_pos2;

                double time_step = abs(next_los_time - next_aos_time) / 50.0;

                for (double ctime = next_aos_time; ctime <= next_los_time; ctime += time_step)
                {
                    predict_orbit(satellite_object, &satellite_orbit2, predict_to_julian_double(ctime));
                    predict_observe_orbit(satellite_observer_station, &satellite_orbit2, &observation_pos2);
                    upcoming_pass_points.push_back({float(observation_pos2.azimuth * RAD_TO_DEG), float(observation_pos2.elevation * RAD_TO_DEG)});
                }
            }
        }

        upcoming_passes_mtx.unlock();
    }
}

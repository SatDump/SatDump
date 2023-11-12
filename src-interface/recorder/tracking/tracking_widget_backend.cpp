#include "tracking_widget.h"
#include "common/tracking/tle.h"
#include "common/utils.h"
#include "logger.h"
#include <cfloat>

namespace satdump
{
    void TrackingWidget::backend_run()
    {
        while (backend_should_run)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            if (!has_tle)
                continue;

            tle_update_mutex.lock();

            if (horizons_mode)
            {
                if (getTime() > last_horizons_fetch_time + 3600)
                {
                    loadHorizons();
                    updateNextPass();
                }

                if (horizons_data.size() > 0)
                {
                    double timed = getTime();

                    int iter = 0;
                    for (int i = 0; i < (int)horizons_data.size(); i++)
                        if (horizons_data[i].timestamp < timed)
                            iter = i;

                    if (getTime() > next_los_time)
                        updateNextPass();

                    current_az = horizons_data[iter].az;
                    current_el = horizons_data[iter].el;
                }
            }
            else
            {
                if (satellite_object != nullptr)
                {
                    predict_orbit(satellite_object, &satellite_orbit, predict_to_julian_double(getTime()));
                    predict_observe_orbit(observer_station, &satellite_orbit, &observation_pos);

                    if (getTime() > next_los_time)
                        updateNextPass();

                    current_az = observation_pos.azimuth * RAD_TO_DEG;
                    current_el = observation_pos.elevation * RAD_TO_DEG;
                }
            }

            // Update
            if (backend_needs_update)
            {
                logger->trace("Updating elements...");

                if (horizons_mode)
                {
                    loadHorizons();
                    updateNextPass();
                }
                else
                {
                    if (satellite_object != nullptr)
                        predict_destroy_orbital_elements(satellite_object);
                    auto &tle = general_tle_registry[current_satellite];
                    satellite_object = predict_parse_tle(tle.line1.c_str(), tle.line2.c_str());
                    updateNextPass();
                }

                backend_needs_update = false;
            }

            processAutotrack();

            tle_update_mutex.unlock();
        }
    }

    void TrackingWidget::updateNextPass()
    {
        logger->trace("Update pass trajectory...");

        upcoming_pass_points.clear();

        next_aos_time = 0;
        next_los_time = 0;

        if (horizons_mode)
        {
            if (horizons_data.size() == 0)
                return;

            upcoming_passes_mtx.lock();

            double timed = getTime();

            int iter = 0;
            for (int i = 0; i < (int)horizons_data.size(); i++)
                if (horizons_data[i].timestamp < timed)
                    iter = i;

            if (horizons_data[iter].el > 0) // Already got AOS
            {
                next_aos_time = timed;

                for (int i = iter - 1; i >= 0; i--) // Attempt to find previous AOS
                {
                    if (horizons_data[i].el <= 0)
                    {
                        next_aos_time = horizons_data[i].timestamp;
                        next_aos_az = horizons_data[i].az;
                        next_aos_el = horizons_data[i].el;
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
                        next_aos_az = horizons_data[i].az;
                        next_aos_el = horizons_data[i].el;
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

            if (next_aos_time != 0 && next_los_time != 0)
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

            upcoming_passes_mtx.unlock();
        }
        else
        {
            if (predict_is_geosynchronous(satellite_object))
            {
                next_aos_time = 0;
                next_los_time = DBL_MAX;
                return;
            }

            upcoming_passes_mtx.lock();

            // Get next LOS
            predict_observation next_aos, next_los;
            next_aos = next_los = predict_next_los(observer_station, satellite_object, predict_to_julian_double(getTime()));

            // Calculate the AOS before that LOS
            next_aos_time = next_los_time = predict_from_julian(next_los.time);
            do
            {
                next_aos = predict_next_aos(observer_station, satellite_object, predict_to_julian_double(next_aos_time));
                next_aos_time -= 10;
            } while (predict_from_julian(next_aos.time) >= next_los_time);

            next_los_time = predict_from_julian(next_los.time);
            next_aos_time = predict_from_julian(next_aos.time);

            next_aos_az = next_aos.azimuth * RAD_TO_DEG;
            next_aos_el = next_aos.elevation * RAD_TO_DEG;

            // Calculate a few points during the pass
            predict_position satellite_orbit2;
            predict_observation observation_pos2;

            double time_step = abs(next_los_time - next_aos_time) / 50.0;

            for (double ctime = next_aos_time; ctime <= next_los_time; ctime += time_step)
            {
                predict_orbit(satellite_object, &satellite_orbit2, predict_to_julian_double(ctime));
                predict_observe_orbit(observer_station, &satellite_orbit2, &observation_pos2);
                upcoming_pass_points.push_back({observation_pos2.azimuth * RAD_TO_DEG, observation_pos2.elevation * RAD_TO_DEG});
            }

            upcoming_passes_mtx.unlock();
        }
    }

    void TrackingWidget::loadHorizons()
    {
        logger->trace("Pulling Horizons data...");

        double curr_time = getTime();
        double start_time = curr_time - 24 * 3600;
        double stop_time = curr_time + 24 * 3600;

        auto hdata = pullHorizonsData(start_time, stop_time, 8640);

        if (hdata.size() > 0)
        {
            horizons_data = hdata;
            last_horizons_fetch_time = curr_time;
            logger->trace("Done pulling Horizons data...");
        }
        else
            logger->trace("Pulled 0 Horizons objects!");
    }

    std::vector<TrackingWidget::HorizonsV> TrackingWidget::pullHorizonsData(double start_time, double stop_time, int num_points)
    {
        std::vector<HorizonsV> hdata;

        std::string cmd = (std::string) "https://ssd.jpl.nasa.gov/api/horizons.api?format=text" +
                          "&OBJ_DATA=NO" +
                          "&MAKE_EPHEM=YES" +
                          "&COMMAND=" + std::to_string(horizonsoptions[current_horizons].first) +
                          "&CAL_FORMAT=JD" +
                          "&EPHEM_TYPE=OBSERVER" +
                          "&CENTER='coord@399'" +
                          "&COORD_TYPE=GEODETIC" +
                          "&SITE_COORD='" + (qth_lon >= 0 ? "+" : "") + std::to_string(qth_lon) + "," +
                          (qth_lat >= 0 ? "+" : "") + std::to_string(qth_lat) + "," +
                          std::to_string(qth_alt / 1e3) + "'" +
                          "&START_TIME='JD " + std::to_string((start_time / 86400.0) + 2440587.5) + "'" +
                          "&STOP_TIME='JD " + std::to_string((stop_time / 86400.0) + 2440587.5) + "'" +
                          "&STEP_SIZE='" + std::to_string(num_points) + "'" + // 86400
                          "&QUANTITIES='4,20'";

        std::string req_result;

        int err = perform_http_request(cmd, req_result);

        if (err != 0)
        {
            logger->error("Could not fetch data from Horizons!");
            return hdata;
        }

        std::istringstream req_results(req_result);
        std::string line;

        bool fount_soe = false;
        bool fount_eoe = false;
        while (getline(req_results, line))
        {
            if (!fount_soe)
            {
                if (line.find("$$SOE") != std::string::npos)
                    fount_soe = true;
                continue;
            }

            if (fount_eoe)
            {
                continue;
            }
            else
            {
                if (line.find("$$EOE") != std::string::npos)
                    fount_eoe = true;
            }

            double julian_time = 0;
            double az = 0;
            double el = 0;
            double delta = 0;
            double deldot = 0;
            double unk = 0;

            if (sscanf(line.c_str(), "%lf%*s    %lf %lf %lf  %lf  %lf",
                       &julian_time, &az, &el, &delta, &deldot, &unk) == 6 ||
                sscanf(line.c_str(), "%lf %*s   %lf %lf %lf  %lf  %*f",
                    &julian_time, &az, &el, &delta, &deldot) == 5 ||
                sscanf(line.c_str(), "%lf %lf %lf %lf  %lf",
                    &julian_time, &az, &el, &delta, &deldot) == 5 ||
                sscanf(line.c_str(), "%lf    %lf %lf %lf  %lf  %lf",
                       &julian_time, &az, &el, &delta, &deldot, &unk) == 6)
            {
                double ctime = (julian_time - 2440587.5) * 86400.0;
                // logger->info("%s %f %f", timestamp_to_string(ctime).c_str(), az, el);
                hdata.push_back({ctime, (float)az, (float)el});
            }
        }

        return hdata;
    }

    std::vector<std::pair<int, std::string>> TrackingWidget::pullHorizonsList()
    {
        std::vector<std::pair<int, std::string>> vv;

        logger->trace("Pulling object list from Horizons...");

        std::string url = "https://ssd.jpl.nasa.gov/api/horizons.api?format=text&COMMAND='*'";
        std::string req_result;
        perform_http_request(url, req_result);

        std::istringstream req_results(req_result);
        std::string line;

        bool got_start = false;
        while (getline(req_results, line))
        {
            if (line.find("  -------  ---------------------------------- -----------  ------------------- ") != std::string::npos)
            {
                got_start = true;
                continue;
            }

            if (!got_start)
                continue;

            try
            {
                std::string numid = line.substr(0, 9);
                std::string name = line.substr(11, 35);
                std::string desig = line.substr(45, 13);
                std::string iauo = line.substr(59, 21);

                int id = std::stoi(numid);

                bool is_valid = false;
                for (auto &c : name)
                    if (c != ' ')
                        is_valid = true;

                if (is_valid && name.find("simulation") == std::string::npos)
                    vv.push_back({id, name});
            }
            catch (std::exception&)
            {
            }
        }

        if (vv.size() > 0)
            return vv;
        else
            return horizonsoptions;
    }
}

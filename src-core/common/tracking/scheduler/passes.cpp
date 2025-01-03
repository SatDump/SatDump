#include "passes.h"
#include "common/geodetic/geodetic_coordinates.h"
#include "libs/predict/predict.h"
#include "logger.h"

namespace satdump
{
    std::vector<SatellitePass> getPassesForSatellite(int norad, double initial_time, double timespan, double qth_lon, double qth_lat, double qth_alt, std::vector<SatellitePass> premade_passes)
    {
        std::vector<SatellitePass> passes;
        predict_observer_t *observer_station = predict_create_observer("Main", qth_lat * DEG_TO_RAD, qth_lon * DEG_TO_RAD, qth_alt * DEG_TO_RAD);
        auto tle = general_tle_registry->get_from_norad(norad);
        if (!tle.has_value())
        {
            logger->warn("NORAD #%d is not available! Skipping pass calculation", norad);
            return passes;
        }

        predict_orbital_elements_t *satellite_object_ = predict_parse_tle(tle->line1.c_str(), tle->line2.c_str());
        double current_time = initial_time;

        if (premade_passes.size() == 0) // Normal algo, for normal LEOs
        {
            while (current_time < initial_time + timespan)
            {
                predict_observation next_aos, next_los;
                next_aos = next_los = predict_next_los(observer_station, satellite_object_, predict_to_julian_double(current_time));

                // Calculate the AOS before that LOS
                double next_aos_time_, next_los_time_;
                next_aos_time_ = next_los_time_ = predict_from_julian(next_los.time);
                do
                {
                    next_aos = predict_next_aos(observer_station, satellite_object_, predict_to_julian_double(next_aos_time_));
                    next_aos_time_ -= 10;
                } while (predict_from_julian(next_aos.time) >= next_los_time_);

                next_los_time_ = predict_from_julian(next_los.time);
                next_aos_time_ = predict_from_julian(next_aos.time);

                float max_el = 0;
                predict_position satellite_orbit2;
                predict_observation observation_pos2;
                double time_step = abs(next_los_time_ - next_aos_time_) / 50.0;
                for (double ctime = next_aos_time_; ctime <= next_los_time_; ctime += time_step)
                {
                    predict_orbit(satellite_object_, &satellite_orbit2, predict_to_julian_double(ctime));
                    predict_observe_orbit(observer_station, &satellite_orbit2, &observation_pos2);
                    if (observation_pos2.elevation * RAD_TO_DEG > max_el)
                        max_el = observation_pos2.elevation * RAD_TO_DEG;
                }

                // if (max_el >= autotrack_min_elevation)
                passes.push_back({norad, next_aos_time_, next_los_time_, max_el});
                // logger->info("Pass of %s at AOS %s LOS %s elevation %.2f", tle.name.c_str(),
                //              timestamp_to_string(next_aos_time_).c_str(),
                //              timestamp_to_string(next_los_time_).c_str(),
                //              max_el);

                current_time = next_los_time_ + 1;
            }
        }
        else // For pre-processed "AOS"/"LOS" times, we just fill in meta
        {
            for (auto pass : premade_passes)
            {
                float max_el = 0;
                predict_position satellite_orbit2;
                predict_observation observation_pos2;
                double time_step = abs(pass.los_time - pass.aos_time) / 50.0;
                for (double ctime = pass.aos_time; ctime <= pass.los_time; ctime += time_step)
                {
                    predict_orbit(satellite_object_, &satellite_orbit2, predict_to_julian_double(ctime));
                    predict_observe_orbit(observer_station, &satellite_orbit2, &observation_pos2);
                    if (observation_pos2.elevation * RAD_TO_DEG > max_el)
                        max_el = observation_pos2.elevation * RAD_TO_DEG;
                }
                pass.norad = norad;
                pass.max_elevation = max_el;
                passes.push_back(pass);
            }
        }

        predict_destroy_orbital_elements(satellite_object_);
        predict_destroy_observer(observer_station);

        return passes;
    }

    std::vector<SatellitePass> filterPassesByElevation(std::vector<SatellitePass> passes, float min_elevation, float max_elevation)
    {
        std::vector<SatellitePass> passes2;

        for (auto &pass : passes)
            if (pass.max_elevation >= min_elevation && pass.max_elevation <= max_elevation)
                passes2.push_back(pass);

        return passes2;
    }

    std::vector<SatellitePass> selectPassesForAutotrack(std::vector<SatellitePass> passes)
    {
        std::vector<SatellitePass> passes2;

#if 0
        for (int i = 0; i < (int)passes.size() - 1; i++)
        {
            auto &pass1 = passes[i];
            auto &pass2 = passes[i + 1];

            if (pass1.los_time > pass2.aos_time)
            {
                // logger->critical("Overlap : ");
                // logPass(pass1);
                // logPass(pass2);

                if (pass1.max_elevation > pass2.max_elevation)
                    passes2.push_back(pass1);
                else
                    passes2.push_back(pass2);
            }
            else
            {
                passes2.push_back(pass1);
            }
        }
#else
        SatellitePass selectedPass = {-1, -1, -1 - 1, -1};
        // double busyUntil = 0;
        if (passes.size() > 0)
        {
            double start_time = passes[0].aos_time;
            double stop_time = passes[passes.size() - 1].los_time;

            for (double current_time = start_time; current_time < stop_time; current_time++)
            {
                std::vector<SatellitePass> ongoing_passes;
                for (auto &cpass : passes)
                    if (cpass.aos_time <= current_time && current_time <= cpass.los_time)
                        ongoing_passes.push_back(cpass);

                if (ongoing_passes.size() == 0)
                    continue;

                // if (current_time < busyUntil)
                //     continue;

                SatellitePass picked_pass = {-1, -1, -1 - 1, -1};
                for (auto &cpass : ongoing_passes)
                    if (picked_pass.max_elevation < cpass.max_elevation)
                        picked_pass = cpass;

                std::vector<SatellitePass> picked_pass_overlaping_passes;
                picked_pass_overlaping_passes.push_back(picked_pass);
                for (auto &cpass : passes)
                    if (picked_pass.aos_time < cpass.los_time && !(picked_pass.los_time <= cpass.aos_time))
                        picked_pass_overlaping_passes.push_back(cpass);

                for (auto &cpass : picked_pass_overlaping_passes)
                    if (picked_pass.max_elevation < cpass.max_elevation)
                        picked_pass = cpass;

                // if (picked_pass.aos_time < selectedPass.los_time)
                //     continue;

                if (picked_pass.norad != selectedPass.norad ||
                    picked_pass.aos_time != selectedPass.aos_time ||
                    picked_pass.los_time != selectedPass.los_time)
                {
                    selectedPass = picked_pass;
                    passes2.push_back(picked_pass);
                    // busyUntil = picked_pass.los_time;
                }
            }
        }
#endif

        return passes2;
    }
}
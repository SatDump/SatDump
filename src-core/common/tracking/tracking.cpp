#include "tracking.h"

extern "C"
{
#include "libs/predict/unsorted.h"
}

namespace satdump
{
    SatelliteTracker::SatelliteTracker(TLE tle)
    {
        satellite_object = predict_parse_tle(tle.line1.c_str(), tle.line2.c_str());
    }

    SatelliteTracker::SatelliteTracker(nlohmann::json ephem)
    {
        std::vector<std::pair<double, double>> xs;
        std::vector<std::pair<double, double>> ys;
        std::vector<std::pair<double, double>> zs;
        std::vector<std::pair<double, double>> vxs;
        std::vector<std::pair<double, double>> vys;
        std::vector<std::pair<double, double>> vzs;

        for (auto e : ephem)
        {
            xs.push_back({e["timestamp"].get<double>(), e["x"].get<double>()});
            ys.push_back({e["timestamp"].get<double>(), e["y"].get<double>()});
            zs.push_back({e["timestamp"].get<double>(), e["z"].get<double>()});
            vxs.push_back({e["timestamp"].get<double>(), e["vx"].get<double>()});
            vys.push_back({e["timestamp"].get<double>(), e["vy"].get<double>()});
            vzs.push_back({e["timestamp"].get<double>(), e["vz"].get<double>()});
            // printf("{%f, %f},\n", e["timestamp"].get<double>(), e["vz"].get<double>());
        }

        interp_x = new LinearInterpolator(xs);
        interp_y = new LinearInterpolator(ys);
        interp_z = new LinearInterpolator(zs);
        interp_vx = new LinearInterpolator(vxs);
        interp_vy = new LinearInterpolator(vys);
        interp_vz = new LinearInterpolator(vzs);
    }

    SatelliteTracker::~SatelliteTracker()
    {
        predict_destroy_orbital_elements(satellite_object);
        if (interp_x != nullptr)
            delete interp_x;
        if (interp_y != nullptr)
            delete interp_y;
        if (interp_z != nullptr)
            delete interp_z;
        if (interp_vx != nullptr)
            delete interp_vx;
        if (interp_vy != nullptr)
            delete interp_vy;
        if (interp_vz != nullptr)
            delete interp_vz;
    }

    geodetic::geodetic_coords_t SatelliteTracker::get_sat_position_at(double utc_time)
    {
        if (satellite_object != nullptr)
        {
            predict_orbit(satellite_object, &satellite_orbit, predict_to_julian_double(utc_time));
        }
        else
        {
            satellite_orbit.time = predict_to_julian_double(utc_time);
            satellite_orbit.position[0] = interp_x->interpolate(utc_time);
            satellite_orbit.position[1] = interp_y->interpolate(utc_time);
            satellite_orbit.position[2] = interp_z->interpolate(utc_time);
            geodetic_t out;
            Calculate_LatLonAlt(satellite_orbit.time, satellite_orbit.position, &out);
            satellite_orbit.latitude = out.lat;
            satellite_orbit.longitude = out.lon;
            satellite_orbit.altitude = out.alt;
        }
        return geodetic::geodetic_coords_t(satellite_orbit.latitude, satellite_orbit.longitude, satellite_orbit.altitude, true).toDegs();
    }

    predict_position SatelliteTracker::get_sat_position_at_raw(double utc_time)
    {
        if (satellite_object != nullptr)
        {
            predict_orbit(satellite_object, &satellite_orbit, predict_to_julian_double(utc_time));
        }
        else
        {
            satellite_orbit.time = predict_to_julian_double(utc_time);
            satellite_orbit.position[0] = interp_x->interpolate(utc_time);
            satellite_orbit.position[1] = interp_y->interpolate(utc_time);
            satellite_orbit.position[2] = interp_z->interpolate(utc_time);
            satellite_orbit.velocity[0] = interp_vx->interpolate(utc_time);
            satellite_orbit.velocity[1] = interp_vy->interpolate(utc_time);
            satellite_orbit.velocity[2] = interp_vz->interpolate(utc_time);
            // printf("%f - %f %f %f - %f %f %f\n", utc_time,
            //        satellite_orbit.position[0], satellite_orbit.position[1], satellite_orbit.position[2],
            //        satellite_orbit.velocity[0], satellite_orbit.velocity[1], satellite_orbit.velocity[2]);
        }
        return satellite_orbit;
    }
}
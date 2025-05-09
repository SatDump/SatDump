#include "calc_azel.h"
#include "common/geodetic/wgs84.h"
#include <cstdio>

namespace geodetic
{
    az_el_coords_t calc_azel(geodetic_coords_t ground_pos, geodetic_coords_t obs)
    {
        /* WGS 84 Constants */
        double radius_e = WGS84::a * 1e3;
        double ecc = WGS84::e;

        // TODOREWORK switch all to meters, including WGS84
        ground_pos.alt *= 1e3;
        obs.alt *= 1e3;

        /* Convert from decimal degrees to radians */
        double lat_ground = (M_PI / 180) * ground_pos.lat;
        double lon_ground = (M_PI / 180) * ground_pos.lon;
        double lat_obs = (M_PI / 180) * obs.lat;
        double lon_obs = (M_PI / 180) * obs.lon;

        /* Add radius of earth to altitudes */
        double r_ground = radius_e + ground_pos.alt;
        double r_obs = radius_e + obs.alt;

        /* WGS 84 Geoid */
        double N = radius_e / sqrt(1 - pow(ecc, 2) * pow(sin(lat_ground), 2));

        /* Convert ground to Earth Centered Rotational (ECR) coordinates */
        double x_ground = (N + ground_pos.alt) * cos(lat_ground) * cos(lon_ground);
        double y_ground = (N + ground_pos.alt) * cos(lat_ground) * sin(lon_ground);
        double z_ground = (N * (1 - pow(ecc, 2)) + ground_pos.alt) * sin(lat_ground);

        /* Convert obv station to Earth Centered Rotational (ECR) coordinates */
        double x_obs = (N + obs.alt) * cos(lat_obs) * cos(lon_obs);
        double y_obs = (N + obs.alt) * cos(lat_obs) * sin(lon_obs);
        double z_obs = (N * (1 - pow(ecc, 2)) + obs.alt) * sin(lat_obs);

        /* Calculate the range vector */
        double range_v_x = x_obs - x_ground;
        double range_v_y = y_obs - y_ground;
        double range_v_z = z_obs - z_ground;

        /* Transform range vector to Topocenteric Horizon */
        double rot_s = sin(lat_ground) * cos(lon_ground) * range_v_x + sin(lat_ground) * sin(lon_ground) * range_v_y - cos(lat_ground) * range_v_z;
        double rot_e = -1 * sin(lon_ground) * range_v_x + cos(lon_ground) * range_v_y;
        double rot_z = cos(lat_ground) * cos(lon_ground) * range_v_x + cos(lat_ground) * sin(lon_ground) * range_v_y + sin(lat_ground) * range_v_z;

        double range = sqrt(pow(rot_s, 2) + pow(rot_e, 2) + pow(rot_z, 2));

        /* Calculate elevation and take care of divide by zero if they're the same point */
        double el = 0;
        if (range == 0)
            el = (M_PI) / 2;
        else
            el = asin(rot_z / range);

        /* Calculate the azmuth and take care of divide by zero */
        double az = 0;
        if (rot_s == 0)
            az = (M_PI) / 2;
        else
            az = atan(-1 * (rot_e / rot_s));

        if (az < 0)
            az = az + (2 * M_PI);

        az_el_coords_t look_here;
        look_here.az = az * (180 / M_PI);
        look_here.el = el * (180 / M_PI);
        look_here.range = range;

        return look_here;
    }
}; // namespace geodetic
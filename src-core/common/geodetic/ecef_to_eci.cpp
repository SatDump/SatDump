#include "ecef_to_eci.h"
#include "geodetic_coordinates.h"
#include "common/geodetic/wgs84.h"

extern "C"
{
#include "libs/predict/unsorted.h"
}

#define JULIAN_TIME_DIFF 2444238.5

namespace
{
    struct vector
    {
        double x;
        double y;
        double z;
    };

    // Must already be in radians!
    void lla2xyz(geodetic::geodetic_coords_t lla, vector &position)
    {
        // double asq = geodetic::WGS84::a * geodetic::WGS84::a;
        double esq = geodetic::WGS84::e * geodetic::WGS84::e;
        double N = geodetic::WGS84::a / sqrt(1 - esq * pow(sin(lla.lat), 2));
        position.x = (N + lla.alt) * cos(lla.lat) * cos(lla.lon);
        position.y = (N + lla.alt) * cos(lla.lat) * sin(lla.lon);
        position.z = ((1 - esq) * N + lla.alt) * sin(lla.lat);
    }

    // Output in radians!
    void xyz2lla(vector position, geodetic::geodetic_coords_t &lla)
    {
        double asq = geodetic::WGS84::a * geodetic::WGS84::a;
        double esq = geodetic::WGS84::e * geodetic::WGS84::e;

        double b = sqrt(asq * (1 - esq));
        double bsq = b * b;
        double ep = sqrt((asq - bsq) / bsq);
        double p = sqrt(position.x * position.x + position.y * position.y);
        double th = atan2(geodetic::WGS84::a * position.z, b * p);
        double lon = atan2(position.y, position.x);
        double lat = atan2((position.z + ep * ep * b * pow(sin(th), 3)), (p - esq * geodetic::WGS84::a * pow(cos(th), 3)));
        // double N = geodetic::WGS84::a / (sqrt(1 - esq * pow(sin(lat), 2)));

        vector g;
        lla2xyz(geodetic::geodetic_coords_t(lat, lon, 0, true), g);

        double gm = sqrt(g.x * g.x + g.y * g.y + g.z * g.z);
        double am = sqrt(position.x * position.x + position.y * position.y + position.z * position.z);
        double alt = am - gm;

        lla = geodetic::geodetic_coords_t(lat, lon, alt, true);
    }
}

void ecef_epehem_to_eci(double time, double &ephem_x, double &ephem_y, double &ephem_z, double &ephem_vx, double &ephem_vy, double &ephem_vz)
{
    time = predict_to_julian_double(time);

    // Calculate LLA from ECEF pos
    geodetic::geodetic_coords_t ephem_lla;
    xyz2lla({ephem_x / 1e3, ephem_y / 1e3, ephem_z / 1e3}, ephem_lla);

    // Calculate LLA from ECEF pos + vel
    geodetic::geodetic_coords_t ephem_llav;
    xyz2lla({(ephem_x + ephem_vx) / 1e3, (ephem_y + ephem_vy) / 1e3, (ephem_z + ephem_vz) / 1e3}, ephem_llav);

    // Calculate position in ECI from LLA
    geodetic_t out;
    out.lon = ephem_lla.lon;
    out.lat = ephem_lla.lat;
    out.alt = ephem_lla.alt;
    double obspos[3];
    double obs_vel[3];
    Calculate_User_PosVel(time + JULIAN_TIME_DIFF, &out, obspos, obs_vel);
    ephem_x = obspos[0];
    ephem_y = obspos[1];
    ephem_z = obspos[2];

    // Calculate position in ECI from LLA
    out.lon = ephem_llav.lon;
    out.lat = ephem_llav.lat;
    out.alt = ephem_llav.alt;
    double obs_vel2[3];
    Calculate_User_PosVel(time + JULIAN_TIME_DIFF, &out, obspos, obs_vel2);
    double ephem_x_v = obspos[0];
    double ephem_y_v = obspos[1];
    double ephem_z_v = obspos[2];

    // Calculate velocity in ECI from the difference, and add Earth-rotation-induced velocity
    ephem_vx = ephem_x_v - ephem_x + obs_vel[0];
    ephem_vy = ephem_y_v - ephem_y + obs_vel[1];
    ephem_vz = ephem_z_v - ephem_z + obs_vel[2];
}

#include "lla_xyz.h"
#include "wgs84.h"

namespace geodetic
{
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
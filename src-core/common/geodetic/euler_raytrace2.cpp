#include "euler_raytrace.h"
#include "wgs84.h"

extern "C"
{
#include "libs/predict/unsorted.h"
}

#define EPSILON 2.2204460492503131e-016

#define JULIAN_TIME_DIFF 2444238.5

namespace geodetic
{
    namespace raytrace_to_earth_namespace
    {
        struct vector
        {
            double x = 0;
            double y = 0;
            double z = 0;

            vector() {}
            vector(double x, double y, double z) : x(x), y(y), z(z) {}

            double distance_square()
            {
                return x * x + y * y + z * z;
            }

            double magnitude()
            {
                return sqrt(x * x + y * y + z * z);
            }

            vector &normalize()
            {
                double mag = magnitude();

                if (mag <= 0)
                    return *this;

                x /= mag;
                y /= mag;
                z /= mag;

                return *this;
            }
        };

        inline vector vectorCross(vector a, vector b)
        {
            vector c;
            c.x = a.y * b.z - a.z * b.y;
            c.y = a.z * b.x - a.x * b.z;
            c.z = a.x * b.y - a.y * b.x;
            return c;
        }

        inline vector rotate_vector_a_around_b(vector a, vector b, double theta)
        {
            double _const = (a.x * b.x + a.y * b.y + a.z * b.z) / (b.x * b.x + b.y * b.y + b.z * b.z);

            vector a_par_b;
            vector a_ortho_b;

            a_par_b.x = _const * b.x;
            a_par_b.y = _const * b.y;
            a_par_b.z = _const * b.z;
            a_ortho_b.x = a.x - a_par_b.x;
            a_ortho_b.y = a.y - a_par_b.y;
            a_ortho_b.z = a.z - a_par_b.z;

            vector w;

            w.x = b.y * a_ortho_b.z - b.z * a_ortho_b.y;
            w.y = b.z * a_ortho_b.x - b.x * a_ortho_b.z;
            w.z = b.x * a_ortho_b.y - b.y * a_ortho_b.x;

            double rnorm_ortho = sqrt(a_ortho_b.x * a_ortho_b.x + a_ortho_b.y * a_ortho_b.y + a_ortho_b.z * a_ortho_b.z);

            double x1 = cos(theta) / rnorm_ortho;
            double x2 = sin(theta) / sqrt(w.x * w.x + w.y * w.y + w.z * w.z);

            vector a_ortho_b_theta;

            a_ortho_b_theta.x = rnorm_ortho * (x1 * a_ortho_b.x + x2 * w.x);
            a_ortho_b_theta.y = rnorm_ortho * (x1 * a_ortho_b.y + x2 * w.y);
            a_ortho_b_theta.z = rnorm_ortho * (x1 * a_ortho_b.z + x2 * w.z);

            vector c;

            c.x = a_ortho_b_theta.x + a_par_b.x;
            c.y = a_ortho_b_theta.y + a_par_b.y;
            c.z = a_ortho_b_theta.z + a_par_b.z;

            return c;
        }
    };

    int raytrace_to_earth_new(double time, const double position_in[3], const double velocity_in[3], geodetic::euler_coords_t pointing, geodetic::geodetic_coords_t &earth_point)
    {
        // Ensure all inputs are in radians
        // position_geo.toRads();
        pointing.toRads();

        // Geodetic coordinates to vector
        raytrace_to_earth_namespace::vector position;
        position.x = position_in[0];
        position.y = position_in[1];
        position.z = position_in[2];

        // Get vector to Nadir
        raytrace_to_earth_namespace::vector nadir_vector;
        {
            geodetic_t out;
            double obspos[3];
            double obs_vel[3];
            Calculate_LatLonAlt(time, position_in, &out);
            out.alt = 0;
            Calculate_User_PosVel(time + JULIAN_TIME_DIFF, &out, obspos, obs_vel);
            // logger->critical("%f-%f %f-%f %f-%f",
            //                  obspos[0], position_in[0],
            //                  obspos[1], position_in[1],
            //                  obspos[2], position_in[2]);
            nadir_vector.x = obspos[0] - position_in[0];
            nadir_vector.y = obspos[1] - position_in[1];
            nadir_vector.z = obspos[2] - position_in[2];
        }

        // Satellite velocity vector
        raytrace_to_earth_namespace::vector velocity_vector;
        velocity_vector.x = velocity_in[0];
        velocity_vector.y = velocity_in[1];
        velocity_vector.z = velocity_in[2];

        // Rotate to apply yaw
        velocity_vector =
            raytrace_to_earth_namespace::rotate_vector_a_around_b(velocity_vector, nadir_vector, pointing.yaw);

        // Compute a vector orthogonal to the velocity vector
        raytrace_to_earth_namespace::vector velocity_vector_90 =
            raytrace_to_earth_namespace::rotate_vector_a_around_b(velocity_vector, nadir_vector, 90 * DEG_TO_RAD);

        // Apply rotations. First, roll
        raytrace_to_earth_namespace::vector pointing_vector =
            raytrace_to_earth_namespace::rotate_vector_a_around_b(nadir_vector, velocity_vector, pointing.roll);

        // Apply rotations. Then, pitch
        pointing_vector =
            raytrace_to_earth_namespace::rotate_vector_a_around_b(pointing_vector, velocity_vector_90, pointing.pitch);

        double u = pointing_vector.x;
        double v = pointing_vector.y;
        double w = pointing_vector.z;

        // WGS84 ellipsoid
        const double &a = geodetic::WGS84::a;
        const double &b = geodetic::WGS84::a;
        const double &c = geodetic::WGS84::b;

        double value = -pow(a, 2) * pow(b, 2) * w * position.z - pow(a, 2) * pow(c, 2) * v * position.y - pow(b, 2) * pow(c, 2) * u * position.x;
        double radical = pow(a, 2) * pow(b, 2) * pow(w, 2) + pow(a, 2) * pow(c, 2) * pow(v, 2) - pow(a, 2) * pow(v, 2) * pow(position.z, 2) +
                         2 * pow(a, 2) * v * w * position.y * position.z - pow(a, 2) * pow(w, 2) * pow(position.y, 2) + pow(b, 2) * pow(c, 2) * pow(u, 2) -
                         pow(b, 2) * pow(u, 2) * pow(position.z, 2) + 2 * pow(b, 2) * u * w * position.x * position.z - pow(b, 2) * pow(w, 2) * pow(position.x, 2) -
                         pow(c, 2) * pow(u, 2) * pow(position.y, 2) + 2 * pow(c, 2) * u * v * position.x * position.y - pow(c, 2) * pow(v, 2) * pow(position.x, 2);
        double magnitude = pow(a, 2) * pow(b, 2) * pow(w, 2) + pow(a, 2) * pow(c, 2) * pow(v, 2) + pow(b, 2) * pow(c, 2) * pow(u, 2);

        if (radical < 0)
            return 1;

        double d = (value - a * b * c * sqrt(radical)) / magnitude;

        if (d < 0)
            return 1;

        position.x += d * u;
        position.y += d * v;
        position.z += d * w;

        // To geodetic
        // xyz2lla(position, earth_point);
        const double position_final[3] = {position.x, position.y, position.z};
        geodetic_t out;
        Calculate_LatLonAlt(time, position_final, &out);

        earth_point = geodetic::geodetic_coords_t(out.lat, out.lon, out.alt / 1000.0, true).toDegs();

        if (earth_point.lon > 180.0)
            earth_point.lon -= 360.0;

        if (std::isnan(earth_point.lat) || std::isnan(earth_point.lon))
            return 1;

        return 0;
    }
};

#include "euler_raytrace.h"
#include "wgs84.h"

#define EPSILON 2.2204460492503131e-016

namespace geodetic
{
    /*
    Namespace to store some stuff used only for this function,
    at least for now.
    I initially wanted to avoid implementing all this, but it
    would have made most of the code much harder and shouldn't
    really impact performances that badly.
    */
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

        vector vectorCross(vector a, vector b)
        {
            vector c;
            c.x = a.y * b.z - a.z * b.y;
            c.y = a.z * b.x - a.x * b.z;
            c.z = a.x * b.y - a.y * b.x;
            return c;
        }

        struct matrix
        {
            double m[16];

            matrix()
            {
                std::fill(m, &m[16], 0);
                m[0] = 1;
                m[5] = 1;
                m[10] = 1;
                m[15] = 1;
            }

            matrix(double a1, double a2, double a3, double a4,
                   double a5, double a6, double a7, double a8,
                   double a9, double a10, double a11, double a12,
                   double a13, double a14, double a15, double a16)
            {
                m[0] = a1;
                m[1] = a2;
                m[2] = a3;
                m[3] = a4;
                m[4] = a5;
                m[5] = a6;
                m[6] = a7;
                m[7] = a8;
                m[8] = a9;
                m[9] = a10;
                m[10] = a11;
                m[11] = a12;
                m[12] = a13;
                m[13] = a14;
                m[14] = a15;
                m[15] = a16;
            }

            double &operator[](const int &i)
            {
                return m[i];
            }

            matrix operator*(const matrix &b)
            {
                matrix out;
                out[0] = m[0] * b.m[0] + m[1] * b.m[4] + m[2] * b.m[8] + m[3] * b.m[12];
                out[1] = m[0] * b.m[1] + m[1] * b.m[5] + m[2] * b.m[9] + m[3] * b.m[13];
                out[2] = m[0] * b.m[2] + m[1] * b.m[6] + m[2] * b.m[10] + m[3] * b.m[14];
                out[3] = m[0] * b.m[3] + m[1] * b.m[7] + m[2] * b.m[11] + m[3] * b.m[15];
                out[4] = m[4] * b.m[0] + m[5] * b.m[4] + m[6] * b.m[8] + m[7] * b.m[12];
                out[5] = m[4] * b.m[1] + m[5] * b.m[5] + m[6] * b.m[9] + m[7] * b.m[13];
                out[6] = m[4] * b.m[2] + m[5] * b.m[6] + m[6] * b.m[10] + m[7] * b.m[14];
                out[7] = m[4] * b.m[3] + m[5] * b.m[7] + m[6] * b.m[11] + m[7] * b.m[15];
                out[8] = m[8] * b.m[0] + m[9] * b.m[4] + m[10] * b.m[8] + m[11] * b.m[12];
                out[9] = m[8] * b.m[1] + m[9] * b.m[5] + m[10] * b.m[9] + m[11] * b.m[13];
                out[10] = m[8] * b.m[2] + m[9] * b.m[6] + m[10] * b.m[10] + m[11] * b.m[14];
                out[11] = m[8] * b.m[3] + m[9] * b.m[7] + m[10] * b.m[11] + m[11] * b.m[15];
                out[12] = m[12] * b.m[0] + m[13] * b.m[4] + m[14] * b.m[8] + m[15] * b.m[12];
                out[13] = m[12] * b.m[1] + m[13] * b.m[5] + m[14] * b.m[9] + m[15] * b.m[13];
                out[14] = m[12] * b.m[2] + m[13] * b.m[6] + m[14] * b.m[10] + m[15] * b.m[14];
                out[15] = m[12] * b.m[3] + m[13] * b.m[7] + m[14] * b.m[11] + m[15] * b.m[15];
                return out;
            }
        };
    };

    /*
    I initially wrote this function in a much, much less elegant way...
    But during my research for some examples of cleaner ways, I stumbled
    upon https://github.com/Digitelektro/MeteorDemod and reused some
    implementations. They were also simplified / cleaned up.
    */
    int raytrace_to_earth(geodetic_coords_t position_geo, euler_coords_t pointing, geodetic_coords_t &earth_point)
    {
        // Ensure all inputs are in radians
        position_geo.toRads();
        pointing.toRads();

        // Generate rotation matrices
        raytrace_to_earth_namespace::matrix rotateX;
        rotateX[5] = cos(-pointing.roll);
        rotateX[6] = -sin(-pointing.roll);
        rotateX[9] = sin(-pointing.roll);
        rotateX[10] = cos(-pointing.roll);

        raytrace_to_earth_namespace::matrix rotateY;
        rotateY[0] = cos(-pointing.pitch);
        rotateY[2] = sin(-pointing.pitch);
        rotateY[8] = -sin(-pointing.pitch);
        rotateY[10] = cos(-pointing.pitch);

        raytrace_to_earth_namespace::matrix rotateZ;
        rotateZ[0] = cos(-pointing.yaw);
        rotateZ[1] = -sin(-pointing.yaw);
        rotateZ[4] = sin(-pointing.yaw);
        rotateZ[5] = cos(-pointing.yaw);

        // Total rotation matrice
        raytrace_to_earth_namespace::matrix rotateXYZ = rotateZ * rotateY * rotateX;

        // Geodetic coordinates to vector
        raytrace_to_earth_namespace::vector position;

        double N = pow(WGS84::a, 2) / sqrt(pow(WGS84::a, 2) * pow(cos(position_geo.lat), 2) + pow(WGS84::b, 2) * pow(sin(position_geo.lat), 2));
        position.x = (N + position_geo.alt) * cos(position_geo.lat) * cos(position_geo.lon);
        position.y = (N + position_geo.alt) * cos(position_geo.lat) * sin(position_geo.lon);
        position.z = ((pow(WGS84::b, 2) / pow(WGS84::a, 2)) * N + position_geo.alt) * sin(position_geo.lat);

        // Matrices
        raytrace_to_earth_namespace::matrix look_matrix;
        raytrace_to_earth_namespace::matrix final_matrix(1, 0, 0, position.x,
                                                         0, 1, 0, position.y,
                                                         0, 0, 1, position.z,
                                                         0, 0, 0, 1);

        raytrace_to_earth_namespace::vector k(-position.x, -position.y, -position.z);
        double m = k.distance_square();
        if (m >= EPSILON)
        {
            k.x *= (1.0 / sqrt(m));
            k.y *= (1.0 / sqrt(m));
            k.z *= (1.0 / sqrt(m));

            raytrace_to_earth_namespace::vector i = raytrace_to_earth_namespace::vectorCross(raytrace_to_earth_namespace::vector(0, 0, 1), k).normalize();
            raytrace_to_earth_namespace::vector j = raytrace_to_earth_namespace::vectorCross(k, i).normalize();

            look_matrix = raytrace_to_earth_namespace::matrix(i.x, j.x, k.x, 0.0,
                                                              i.y, j.y, k.y, 0.0,
                                                              i.z, j.z, k.z, 0.0,
                                                              0.0, 0.0, 0.0, 1.0);
        }

        final_matrix = final_matrix * look_matrix * rotateXYZ;

        // Vector
        double u = final_matrix[2];
        double v = final_matrix[6];
        double w = final_matrix[10];

        // WGS84 ellipsoid
        const double &a = WGS84::a;
        const double &b = WGS84::a;
        const double &c = WGS84::b;

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
        double p = sqrt(pow(position.x, 2) + pow(position.y, 2));
        double phi = atan2(position.z * WGS84::a, p * WGS84::b);
        double lat = atan2(position.z + pow(WGS84::e2, 2) * WGS84::b * pow(sin(phi), 3), p - pow(WGS84::e, 2) * WGS84::a * pow(cos(phi), 3));
        double lon = atan2(position.y, position.x);

        earth_point = geodetic::geodetic_coords_t(lat, lon, 0, true);

        return 0;
    }
};

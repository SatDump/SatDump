/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "common/geodetic/calc_azel.h"
#include "common/geodetic/geodetic_coordinates.h"
#include "common/geodetic/wgs84.h"
#include "common/utils.h"
#include "logger.h"
#include "nutation.h"
#include <calceph.h>
#include <cstdio>
#include <sstream>
#include <unistd.h>

#if 1
extern "C"
{
#include <novas.h>

#include <novas-calceph.h>
#include <novas-cspice.h>
}

// Below are some Earth orientation values. Here we define them as constants, but they may
// of course be variables. They should be set to the appropriate values for the time
// of observation based on the IERS Bulletins...

#define LEAP_SECONDS 37            ///< [s] current leap seconds from IERS Bulletin C
#define DUT1 0.114                 ///< [s] current UT1 - UTC time difference from IERS Bulletin A
#define POLAR_DX (0.0863 / 1000.0) // 230.0 ///< [mas] Earth polar offset x, e.g. from IERS Bulletin A.
#define POLAR_DY (0.4178 / 1000.0) // -62.0             ///< [mas] Earth polar offset y, e.g. from IERS Bulletin A.
#endif

#define ENABLE_CUSTOM_AZ_EL 1

#if ENABLE_CUSTOM_AZ_EL
namespace t
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
} // namespace t
#endif

int main(int argc, char *argv[])
{
    initLogger();

    logger->trace("NOVAS_TEST\n");

#if 1
    // SuperNOVAS variables used for the calculations ------------------------->
    novas_orbital orbit = NOVAS_ORBIT_INIT; // Orbital parameters
    object source;                          // a celestial object: sidereal, planet, ephemeris or orbital source
    observer obs;                           // observer location
    novas_timespec obs_time;                // astrometric time of observation
    novas_frame obs_frame;                  // observing frame defined for observing time and location
    enum novas_accuracy accuracy;           // NOVAS_FULL_ACCURACY or NOVAS_REDUCED_ACCURACY
    sky_pos apparent;                       // calculated precise observed (apparent) position of source

    // Calculated quantities ------------------------------------------------->
    double az, el; // calculated azimuth and elevation at observing site

    // Intermediate variables we'll use -------------------------------------->
    struct timespec unix_time; // Standard precision UNIX time structure

    // We'll print debugging messages and error traces...
    novas_debug(NOVAS_DEBUG_ON);

#if 0
    // -------------------------------------------------------------------------
    // We'll use the CALCEPH library to provide ephemeris data

    // First open one or more ephemeris files with CALCEPH to use
    // E.g. the DE440 (short-term) ephemeris data from JPL.
    const char *arrr[2] = {"/home/alan/Downloads/de440s.bsp", "/home/alan/Downloads/jwst_pred.bsp"}; //*/ "/home/alan/Downloads/juice_orbc_000082_230414_310721_v01.bsp"};
    t_calcephbin *de440 = calceph_open_array(2, arrr);                                               // calceph_open("/home/alan/Downloads/de440s.bsp");
    if (!de440)
    {
        fprintf(stderr, "ERROR! could not open ephemeris data\n");
        return 1;
    }

#if 0
    // Try to list
    {
        struct ObjEntry
        {
        };

        int count = calceph_getpositionrecordcount(de440);
        logger->info(count);
        for (int i = 0; i < count; i++)
        {
            int target;
            int center;
            double firsttime;
            double lasttime;
            int frame;
            calceph_getpositionrecordindex(de440, i, &target, &center, &firsttime, &lasttime, &frame);

            char objname[40];
            calceph_getnamebyidss(de440, target, 0, objname);

            char timestamp1[40], timestamp2[40];
            novas_timespec timespec1, timespec2;
            novas_set_time(novas_timescale::NOVAS_TAI, firsttime, LEAP_SECONDS, DUT1, &timespec1);
            novas_set_time(novas_timescale::NOVAS_TAI, lasttime, LEAP_SECONDS, DUT1, &timespec2);

            novas_iso_timestamp(&timespec1, timestamp1, sizeof(timestamp1));
            novas_iso_timestamp(&timespec2, timestamp2, sizeof(timestamp2));

            logger->info("- Body ((%s)) : ID %d || %s %s", std::string(objname).c_str(), target, timestamp1, timestamp2);
        }
    }
#endif

    // Make de440 provide ephemeris data for the major planets.
    novas_use_calceph(de440);
#elif 1
    cspice_add_kernel("/home/alan/Downloads/de440s.bsp");
    cspice_add_kernel("/home/alan/Downloads/juice_orbc_000082_230414_310721_v01.bsp");
    cspice_add_kernel("/home/alan/Downloads/jwst_pred.bsp");

    novas_use_cspice();
#endif

    // Orbitals assume Keplerian motion, and are never going to be accurate much below the
    // tens of arcsec level even for the most current MPC orbits. Orbitals for planetary
    // satellites are even less precise. So, with orbitals, there is no point on pressing
    // for ultra-high (sub-uas level) accuracy...
    accuracy = NOVAS_FULL_ACCURACY; // NOVAS_REDUCED_ACCURACY; // mas-level precision, typically

    // -------------------------------------------------------------------------
    // Define a sidereal source

#if 0
                                    // Orbital Parameters for the asteroid Ceres from the Minor Planet Center
    // (MPC) at JD 2460600.5
    orbit.jd_tdb = 2460600.5; // [day] TDB date
    orbit.a = 2.7666197;      // [AU]
    orbit.e = 0.079184;
    orbit.i = 10.5879;      // [deg]
    orbit.omega = 73.28579; // [deg]
    orbit.Omega = 80.25414; // [deg]
    orbit.M0 = 145.84905;   // [deg]
    orbit.n = 0.21418047;   // [deg/day]

    // Define Ceres as the observed object (we can use whatever ID numbering
    // system here, since it's irrelevant to SuperNOVAS in this context).
    make_orbital_object("Ceres", 2000001, &orbit, &source);
#elif 0
    // ... Or, you could define orbitals for a satellite instead:

    // E.g. Callisto's orbital parameters from JPL Horizons
    // https://ssd.jpl.nasa.gov/sats/elem/sep.html
    // 1882700. 0.007 43.8  87.4  0.3 309.1 16.690440 277.921 577.264 268.7 64.8
    orbit.system.center = NOVAS_JUPITER;
    novas_set_orbsys_pole(NOVAS_GCRS, 268.7 / 15.0, 64.8, &orbit.system);

    orbit.jd_tdb = NOVAS_JD_J2000;
    orbit.a = 1882700.0 * 1e3 / NOVAS_AU;
    orbit.e = 0.007;
    orbit.omega = 43.8;
    orbit.M0 = 87.4;
    orbit.i = 0.3;
    orbit.Omega = 309.1;
    orbit.n = TWOPI / 16.690440;
    orbit.apsis_period = 277.921 * 365.25;
    orbit.node_period = 577.264 * 365.25;

    // Set Callisto as the observed object
    make_orbital_object("Callisto", 501, &orbit, &source);
#elif 1
    // make_planet(NOVAS_MOON, &source);
    // make_planet(NOVAS_SUN, &source);
    //     make_ephem_object("STEREO-A", -234, &source);
    //  make_ephem_object("Io", 501, &source);
    //  make_ephem_object("JWST", -170, &source);
    make_ephem_object("JUICE", -28, &source);
#endif

    // -------------------------------------------------------------------------
    // Define observer somewhere on Earth (we can also define observers in Earth
    // or Sun orbit, at the geocenter or at the Solary-system barycenter...)

    // Specify the location we are observing from
    // 50.7374 deg N, 7.0982 deg E, 60m elevation
    // (We'll ignore the local weather parameters here, but you can set those too.)
    if (make_observer_at_geocenter(&obs))
    // if (make_observer_on_surface(48.0, 1.8, 173.0, 0.0, 0.0, &obs) != 0)
    {
        fprintf(stderr, "ERROR! defining Earth-based observer location.\n");
        return 1;
    }

    observer obs2;
    make_observer_on_surface(48.0, 1.8, 173.0, 0.0, 0.0, &obs2);

    while (1)
    {
        // -------------------------------------------------------------------------
        // Set the astrometric time of observation...

        // Get the current system time, with up to nanosecond resolution...
        clock_gettime(CLOCK_REALTIME, &unix_time);

        // Set the time of observation to the precise UTC-based UNIX time
        // (We can set astromtric time using an other time measure also...)
        if (novas_set_unix_time(unix_time.tv_sec, unix_time.tv_nsec, LEAP_SECONDS, DUT1, &obs_time) != 0)
        {
            fprintf(stderr, "ERROR! failed to set time of observation.\n");
            return 1;
        }

        // ... Or you could set a time explicily in any known timescale.
        /*
        // Let's set a TDB-based time for the start of the J2000 epoch exactly...
        if(novas_set_time(NOVAS_TDB, NOVAS_JD_J2000, 32, 0.0, &obs_time) != 0) {
          fprintf(stderr, "ERROR! failed to set time of observation.\n");
          return 1;
        }
        */

        // -------------------------------------------------------------------------
        // You might want to set a provider for precise planet positions so we might
        // calculate Earth, Sun and major planet positions accurately. It is needed
        // if you have orbitals defined around a major planet.
        //
        // There are many ways to set a provider of planet positions. For example,
        // you may use the CALCEPH library:
        //
        // t_calcephbin *planets = calceph_open("path/to/de440s.bsp");
        // novas_use_calceph(planets);

        // -------------------------------------------------------------------------
        // Initialize the observing frame with the given observing and Earth
        // orientation patameters.
        //
        if (novas_make_frame(accuracy, &obs, &obs_time, POLAR_DX, POLAR_DY, &obs_frame) != 0)
        {
            fprintf(stderr, "ERROR! failed to define observing frame.\n");
            return 1;
        }

        // -------------------------------------------------------------------------
        // Calculate the precise apparent position (e.g. in CIRS).
        if (novas_sky_pos(&source, &obs_frame, NOVAS_CIRS, &apparent) != 0)
        {
            fprintf(stderr, "ERROR! failed to calculate apparent position.\n");
            return 1;
        }

        // Let's print the apparent position
        // (Note, CIRS R.A. is relative to CIO, not the true equinox of date.)
        printf(" RA = %.9f deg, Dec = %.9f deg, rad_vel = %.6f km/s, distance = %.6f ", apparent.ra * 15., apparent.dec, apparent.rv, (apparent.dis * NOVAS_AU) / 1e3);

        // -------------------------------------------------------------------------
        // Convert the apparent position in CIRS on sky to horizontal coordinates
        // We'll use a standard (fixed) atmospheric model to estimate an optical refraction
        // (You might use other refraction models, or NULL to ignore refraction corrections)
        //     if (novas_app_to_hor(&obs_frame, NOVAS_CIRS, apparent.ra, apparent.dec, novas_standard_refraction, &az, &el) != 0)
        //    {
        //        fprintf(stderr, "ERROR! failed to calculate azimuth / elevation.\n");
        //       return 1;
        //   }

        double jd_tt = novas_get_time(&obs_time, novas_timescale::NOVAS_TT);

#if ENABLE_CUSTOM_AZ_EL
        double pos[3], vel[3];
        // if (novas_orbit_posvel((obs_time.ijd_tt + obs_time.fjd_tt) + obs_time.tt2tdb, &orbit, NOVAS_FULL_ACCURACY, pos,vel))
        if (novas_geom_posvel(&source, &obs_frame, novas_reference_system::NOVAS_CIRS, pos, vel))
        {
            fprintf(stderr, "ERROR! failed to calculate pos / vel.\n");
            return 1;
        }

        double pos2[3];
        if (cirs_to_itrs(/*double jd_tt_high*/ jd_tt, /*double jd_tt_low*/ 0, obs_time.ut1_to_tt, NOVAS_FULL_ACCURACY, POLAR_DX * 1e3, POLAR_DY * 1e3, pos, pos2))
        {
            fprintf(stderr, "ERROR! failed to calculate CIRS => ITRS.\n");
            return 1;
        }

        geodetic::geodetic_coords_t c;
        t::xyz2lla({(pos2[0] * NOVAS_AU) / 1e3, (pos2[1] * NOVAS_AU) / 1e3, (pos2[2] * NOVAS_AU) / 1e3}, c);
        c.toDegs();

        // printf(" X %.6f Y %.6f Z %.6f (%.6f, %.6f, %.6f Km)", (pos2[0] * NOVAS_AU) / 1e3, (pos2[1] * NOVAS_AU) / 1e3, (pos2[2] * NOVAS_AU) / 1e3, c.lat, c.lon, c.alt);

        //   novas_track tr;
        //   novas_equ_track(&source, &obs_frame, 0.00001, &tr);
        //   printf(" Lat = %.2f deg, Lon = %.6f deg ", tr.pos.lat, tr.pos.lon);

        geodetic::geodetic_coords_t obs(48.0, 1.8, 173.0 / 1e3);
        auto v = geodetic::calc_azel(obs, c);
        printf(" Az2 = %.6f deg, El2 = %.6f deg ||| ", v.az, v.el);
#endif

        /*double dx, dy;
        // iau2000a(jd_tt, 0, &dx, &dy);
        double jd_tt2 = novas_get_time(&obs_time, novas_timescale::NOVAS_TT);
        nutation_angles((jd_tt2 - 2451545.0) / 36525.0, NOVAS_FULL_ACCURACY, &dx, &dy);
        printf("JD %f DX %f %f, DY %f %f ", jd_tt, dx * NOVAS_ARCSEC, POLAR_DX, dy * NOVAS_ARCSEC, POLAR_DY);
*/

        novas_frame obs_frame2;
        novas_make_frame(accuracy, &obs2, &obs_time, POLAR_DX, POLAR_DY, &obs_frame2);
        sky_pos apparent2;
        novas_sky_pos(&source, &obs_frame2, NOVAS_CIRS, &apparent2);
        novas_app_to_hor(&obs_frame2, NOVAS_CIRS, apparent2.ra, apparent2.dec, NULL, &az, &el);
        // Let's print the calculated azimuth and elevation
        printf(" Az = %.6f deg, El = %.6f deg\n", az, el);

        // sleep(1);
    }
#endif
    return 0;
}

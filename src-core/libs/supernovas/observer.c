/**
 * @file
 *
 * @date Created  on Mar 6, 2025
 * @author Attila Kovacs and G. Kaplan
 *
 *  This module provides a set of functions that define an astronomical observer location, or ones
 *  that relate to observer location.
 *
 *  The following type of observer locations are supported:
 *
 *  1. Geodetic Earth-based observer locations:
 *    a. ground-based observer sites:
 *       - GPS locations via `make_gps_observer()`
 *       - ITRF / GRS80 locations via `make_itrs_observer()`
 *       - Cartesian _x_, _y_, _z_ coordinates via `make_xyz_site()` and `make_site_observer()`
 *    b. airborne observers, via a location (e.g. `make_gps_site()`, `make_itrf_site()` or
 *       `make_xyz_site()`), and a ground velocity using `make_airborne_observer()`.
 *
 *  2. Near Earth Orbit (NEO) locations, via `make_observer_in_space()` specifying momentary
 *     position and velocity w.r.t. the geocenter.
 *
 *  3. Virtual observer at the geocenter via `make_observer_at_geocecenter()`.
 *
 *  4. Solar-system locations via `make_solar_system_observer()`, specifying a momentary
 *     barycentric (that is w.r.t. the SSB) position and velocity vector.
 *
 * Once an observer is defined, it maybe used to set up an observing frame for a specific time of
 * observation. Observing frames allow efficient and precise position calculations from an
 * observer's point-of-view.
 *
 * This module also provides functions otherwise relating to the observer locations.
 *
 * For Earth-based geodetic locations, the module provides a set of functions to convert between
 * various (local) coordinate systems. For example, calculating parallactic angles (a.k.a.
 * vertical position angle or VPA), for converting between horizontal and equatorial offsets,
 * and various functions for converting equatorial vectors to a line-of-sight, or _u_, _v_, _w_
 * coordinate bases, and vice-versa.
 *
 * @sa timescale.c, frame.c
 */

#include <string.h>
#include <errno.h>

/// \cond PRIVATE
#define __NOVAS_INTERNAL_API__    ///< Use definitions meant for internal use by SuperNOVAS only
#include "novas.h"

#define MOIST_ADIABATIC_LAPSE_RATE        0.005     ///< [C/m] Moist adiabatic lapse rate (see Feulner et al. 2013).

#define ANTARCTIC_ADIABATIC_LAPSE_RATE    0.0098    ///< [C/m] Moist adiabatic lapse rate (see Feulner et al. 2013).

#define ANTARCTIC_LATITUDE                (-66.55)  ///< [deg] latitude below which to use Antarctic lapse rate

#define PRESSURE_SCALE_HEIGHT             9.1e3     ///< [m] Scale height of atmospheric pressure


/// \endcond

/**
 * @deprecated    It is recommended that you use one of the more specific ways of initializing
 *                the observer data structure, e.g. with `make_itrf_observer()`,
 *                `make_gps_observer()`, `make_observer_at_site()`, `make_airborne_observer()`
 *                `make_solar_system_observer(), or `make_observer_at_geocenter()`. This function
 *                will be available for the foreseeable future also.
 *
 * Populates an 'observer' data structure given the parameters. The output data structure may
 * be used an the the inputs to NOVAS-C functions, such as `make_frame()` or `place()`.
 *
 * @param where         The location type of the observer
 * @param loc_surface   Pointer to data structure that defines a location on Earth's surface.
 *                      Used only if 'where' is NOVAS_OBSERVER_ON_EARTH, otherwise can be
 *                      NULL.
 * @param loc_space     Pointer to data structure that defines a near-Earth location in space.
 *                      Used only if 'where' is NOVAS_OBSERVER_IN_EARTH_ORBIT, otherwise can
 *                      be NULL.
 * @param[out] obs      Pointer to observer data structure to populate.
 * @return              0 if successful, -1 if a required argument is NULL, or 1 if the 'where'
 *                      argument is invalid.
 *
 * @sa make_observer_at_geocenter(), make_itrf_observer(), make_gps_observer(),
 *     make_airborne_observer(), make_observer_in_space(), make_solar_system_observer()
 */
short make_observer(enum novas_observer_place where, const on_surface *loc_surface, const in_space *loc_space, observer *obs) {
  static const char *fn = "make_observer";

  if(!obs)
    return novas_error(-1, EINVAL, fn, "NULL output observer pointer");

  // Initialize the output structure.
  memset(obs, 0, sizeof(*obs));
  obs->where = where;

  // Populate the output structure based on the value of 'where'.
  switch(where) {
    case NOVAS_OBSERVER_AT_GEOCENTER:
      break;

    case NOVAS_AIRBORNE_OBSERVER:
      if(!loc_space)
        return novas_error(-1, EINVAL, fn, "NULL in space location (for velocity)");

      memcpy(&obs->near_earth.sc_vel, loc_space->sc_vel, sizeof(loc_space->sc_vel)); // @suppress("No break at end of case")
      /* fallthrough */

    case NOVAS_OBSERVER_ON_EARTH:
      if(!loc_surface)
        return novas_error(-1, EINVAL, fn, "NULL on surface location");

      memcpy(&obs->on_surf, loc_surface, sizeof(obs->on_surf));
      break;

    case NOVAS_OBSERVER_IN_EARTH_ORBIT:
    case NOVAS_SOLAR_SYSTEM_OBSERVER:
      if(!loc_space)
        return novas_error(-1, EINVAL, fn, "NULL in space location");

      memcpy(&obs->near_earth, loc_space, sizeof(obs->near_earth));
      break;

    default:
      return novas_error(1, EINVAL, fn, "Invalid observer location type (where): %d", where);
  }

  return 0;
}

/**
 * Populates an 'observer' data structure for a hypothetical observer located at Earth's
 * geocenter. The output data structure may be used an the the inputs to NOVAS-C functions,
 * such as `make_frame()` or `place()`.
 *
 * @param[out] obs    Pointer to data structure to populate.
 * @return          0 if successful, or -1 if the output argument is NULL.
 *
 * @sa make_gps_observer(), make_itrf_observer(), make_observer_at_site(),
 *     make_airborne_observer(), make_observer_in_space(), make_solar_system_observer()
 * @sa novas_make_frame()
 */
int make_observer_at_geocenter(observer *restrict obs) {
  prop_error("make_observer_at_geocenter", make_observer(NOVAS_OBSERVER_AT_GEOCENTER, NULL, NULL, obs), 0);
  return 0;
}

/**
 * @deprecated This old NOVAS C function has a few too many caveats. It is recommended that you
 *             use `make_itrf_observer()`, `make_gps_observer()`, or `make_observer_at_site()`
 *             instead (all of which set default mean annual weather parameters for approximate
 *             refraction correction), and optionally set actual weather data afterwards, based on
 *             the measurements available. This function will be available for the foreseeable
 *             future also.
 *
 * Initializes an observer data structure with the specified ITRF / GRS80 location, and he
 * specified local pressure and temperature. This old NOVAS C function does not set humidity.
 * Thus, if humidity is needed for refraction correction, then you should set it explicitly after
 * this call.
 *
 * NOTES:
 * <ol>
 * <li>You can convert coordinates among ITRF realization using `novas_itrf_transform()`,
 * possibly after `novas_geodetic_to_cartesian()` with `NOVAS_GRS80_ELLIPSOID` as necessary for
 * polar ITRF coordinates.</li>
 * <li>You can convert ITRF Cartesian _xyz_ locations to geodetic locations by using
 * `novas_cartesian_to_geodetic()` with `NOVAS_GRS80_ELLIPSOID` as the reference ellipsoid
 * parameter.</li>
 * <li>If you have longitude, latitude, and height defined as GPS (WGS84) values, you might want
 * to use make_gps_observer() intead</li>
 * </ol>
 *
 * @param latitude      [deg] Geodetic (ITRF / GRS80) latitude; north positive.
 * @param longitude     [deg] Geodetic (ITRF / GRS80) longitude; east positive.
 * @param height        [m] Geodetic (ITRF / GRS80) altitude above sea level of the observer.
 * @param temperature   [C] Temperature (degrees Celsius).
 * @param pressure      [mbar] Atmospheric pressure.
 * @param[out] obs      Pointer to the data structure to populate.
 *
 * @return              0 if successful, or -1 if the output argument is NULL, or if the latitude
 *                      is outside of the [-90:90] range, or if the temperature or pressure values
 *                      are impossible for an Earth based observer (errno set to ERANGE).
 *
 * @sa make_itrf_observer(), make_gps_observer(), make_observer_at_site()
 * @sa novas_set_weather(), novas_cartesian_to_geodetic(), novas_geodetic_to_cartesian(),
 *     NOVAS_GRS80_ELLIPSOID, novas_make_frame()
 */
int make_observer_on_surface(double latitude, double longitude, double height, double temperature, double pressure,
        observer *restrict obs) {
  static const char *fn = "make_observer_on_surface";
  on_surface loc;
  prop_error(fn, make_on_surface(latitude, longitude, height, temperature, pressure, &loc), 0);
  prop_error(fn, make_observer(NOVAS_OBSERVER_ON_EARTH, &loc, NULL, obs), 0);
  return 0;
}

/**
 * Initializes an observer data structure for a ground-based observer at the specified observing
 * site, and sets mean (annual) weather parameters based on that location.
 *
 * @param site          Pointer to observing site data structure.
 * @param[out] obs      Pointer to the data structure to populate.
 *
 * @return              0 if successful, or -1 if the either argument is NULL (errno set to
 *                      EINVAL).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa make_itrf_observer(), make_gps_observer(), make_airborne_observer(),
 *     make_observer_in_space(), make_observer_at_geocenter(), make_solar_system_observer()
 * @sa make_itrf_site(), make_gps_site(), make_xyz_site(), novas_make_frame()
 */
int make_observer_at_site(const on_surface *restrict site, observer *restrict obs) {
  static const char *fn = "make_observer_at_site";

  if(!site)
    return novas_error(-1, EINVAL, fn, "input site is NULL");

  if(!obs)
    return novas_error(-1, EINVAL, fn, "output observer is NULL");

  obs->where = NOVAS_OBSERVER_ON_EARTH;
  obs->on_surf = *site;

  return 0;
}

/**
 * Initializes an observer data structure for a ground-based observer with the specified
 * International Terrestrial Reference Frame (ITRF) / GRS80 location, and sets mean (annual)
 * weather parameters based on that location.
 *
 * For the highest precision (&mu;as level) applications you should make sure that the location
 * provided here and the Earth-orientation parameters (EOP) used (in setting `novas_timescale`
 * and in `novas_make_frame()`) are provided in the same ITRF realization. You can use
 * `novas_itrf_transform_eop()` to change the ITRF realization for the EOP values,
 * if necessary.
 *
 * @param latitude      [deg] Geodetic (ITRF / GRS80) latitude; north positive.
 * @param longitude     [deg] Geodetic (ITRF / GRS80) longitude; east positive.
 * @param height        [m] Geodetic (ITRF / GRS80) altitude above sea level of the observer.
 * @param[out] obs      Pointer to the data structure to populate.
 *
 * @return              0 if successful, or -1 if the output argument is NULL (errno set to
 *                      EINVAL), or if the latitude is outside of the [-90:90] range (errno
 *                      set to ERANGE).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa make_gps_observer(), make_observer_at_site(), make_airborne_observer(),
 *     make_observer_in_space(), make_observer_at_geocenter(), make_solar_system_observer()
 * @sa make_itrf_site(), novas_itrf_transform_site(), novas_set_default_weather(),
 *     novas_make_frame()
 */
int make_itrf_observer(double latitude, double longitude, double height, observer *obs) {
  on_surface site; memset(&site, 0, sizeof(on_surface));
  make_itrf_site(latitude, longitude, height, &site);
  prop_error("make_itrf_observer", make_observer_at_site(&site, obs), 0);
  return 0;
}

/**
 * Initializes an observer data structure for a ground-based observer with the specified GPS /
 * WGS84 location, and sets mean (annual) weather parameters based on that location.
 *
 * For the highest (&mu;as / mm level) precision, you probably should use an ITRF location
 * instead of a GPS based location.
 *
 * @param latitude      [deg] Geodetic (GPS / WGS84) latitude north positive.
 * @param longitude     [deg] Geodetic (GPS / WGS84) longitude east positive.
 * @param height        [m] Geodetic (GPS / WGS84) altitude above sea level of the observer.
 * @param[out] obs      Pointer to the data structure to populate.
 *
 * @return              0 if successful, or -1 if the output argument is NULL (errno set to
 *                      EINVAL), or if the latitude is outside of the [-90:90] range (errno
 *                      set to ERANGE).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa make_itrf_observer(), make_observer_at_site(), make_airborne_observer(),
 *     make_observer_in_space(), make_observer_at_geocenter(), make_solar_system_observer()
 * @sa make_gps_site(), novas_set_default_weather(), novas_make_frame(),
 *     novas_geodetic_transform_site()
 */
int make_gps_observer(double latitude, double longitude, double height, observer *obs) {
  on_surface site; memset(&site, 0, sizeof(on_surface));
  make_gps_site(latitude, longitude, height, &site);
  prop_error("make_gps_observer", make_observer_at_site(&site, obs), 0);
  return 0;
}

/**
 * Populates an 'observer' data structure, for an observer situated on a near-Earth spacecraft,
 * with the specified geocentric position and velocity vectors. Both input vectors are with
 * respect to true equator and equinox of date. The output data structure may be used an the
 * the inputs to NOVAS-C functions, such as `make_frame()` or `place()`.
 *
 * @param sc_pos        [km] Geocentric (x, y, z) position vector.
 * @param sc_vel        [km/s] Geocentric (x, y, z) velocity vector.
 * @param[out] obs      Pointer to the data structure to populate
 * @return          0 if successful, or -1 if the output argument is NULL.
 *
 * @sa make_gps_observer(), make_itrf_observer(), make_observer_at_site(),
 *     make_airborne_observer(), make_observer_at_geocenter(), make_solar_system_observer()
 * @sa novas_make_frame()
 */
int make_observer_in_space(const double *sc_pos, const double *sc_vel, observer *obs) {
  static const char *fn = "make_observer_in_space";
  in_space loc;
  prop_error(fn, make_in_space(sc_pos, sc_vel, &loc), 0);
  prop_error(fn, make_observer(NOVAS_OBSERVER_IN_EARTH_ORBIT, NULL, &loc, obs), 0);
  return 0;
}

/**
 * @deprecated This old NOVAS C function has a few too many caveats. It is recommended that you
 *             use `make_itrf_site()` or `make_gps_site()` instead (both of which set default
 *             mean annual weather parameters for approximate refraction correction), and
 *             optionally set actual weather data afterwards, based on the measurements
 *             available. This function will be available for the foreseeable future also.
 *
 * Populates an 'on_surface' data structure, for an observer on the surface of the Earth, with the
 * given parameters.
 *
 * Note, that because this is an original NOVAS C routine, it does not have an argument to set a
 * humidity value (e.g. for radio refraction). As such, the humidity is set to a a default mean
 * annual value for the location. To set an actual humidity, set the output structure's field
 * after calling this funcion.
 *
 * NOTES
 * <ol>
 * <li>This implementation breaks strict v1.0 ABI compatibility since it writes to (initializes)
 * a field (`humidity`) that was not yet part of the `on_surface` structure in v1.0. As such,
 * linking SuperNOVAS v1.1 or later with application code compiled for SuperNOVAS v1.0 can result
 * in memory corruption or segmentation fault when this function is called. To be safe, make sure
 * your application has been (re)compiled against SuperNOVAS v1.1 or later.</li>
 * <li>You can convert coordinates among ITRF realization using `novas_itrf_transform()`,
 * possibly after `novas_geodetic_to_cartesian()` with `NOVAS_GRS80_ELLIPSOID` as necessary for
 * polar ITRF coordinates.</li>
 * <li>You can convert Cartesian _xyz_ locations to geodetic locations by using
 * `novas_cartesian_to_geodetic()` with `NOVAS_GRS80_ELLIPSOID` as the reference
 * ellipsoid parameter.</li>
 * <li>If you have longitude, latitude, and height defined as GPS (WGS84) values, you might want
 * to use `make_gps_site()` instead, and then set weather parameters afterwards as necessary.
 * </li>
 * </ol>
 *
 * @param latitude      [deg] Geodetic (ITRF / GRS80) latitude; north positive.
 * @param longitude     [deg] Geodetic (ITRF / GRS80) longitude; east positive.
 * @param height        [m] Geodetic (ITRF / GSR80) altitude above sea level of the observer.
 * @param temperature   [C] Temperature (degrees Celsius) [-120:70].
 * @param pressure      [mbar] Atmospheric pressure [0:1200].
 * @param[out] loc      Pointer to Earth location data structure to populate.
 *
 * @return          0 if successful, or -1 if the output argument is NULL (errno set to EINVAL),
 *                  or if the latitude is outside of the [-90:90] range, or if the temperature
 *                  or pressure values are impossible for an Earth based observer (errno set to
 *                  ERANGE).
 *
 * @sa make_itrf_site(), make_gps_site(), make_xyz_site()
 * @sa novas_set_default_weather(), novas_cartesian_to_geodetic(), ON_SURFACE_INIT, ON_SURFACE_LOC,
 *     NOVAS_GRS80_ELLIPSOID
 */
int make_on_surface(double latitude, double longitude, double height, double temperature, double pressure,
        on_surface *restrict loc) {
  static const char *fn = "make_on_surface";

  // mesosphere can be -100C, highest recorded atmospheric temperature is 56.7C...
  if(temperature < -120.0 || temperature > 70.0)
    return novas_error(-1, ERANGE, fn, "impossible ambient temperature: %g celsius", temperature);

  // highest on record is 1083.8 mbar...
  if(pressure < 0.0 || pressure > 1200.0)
    return novas_error(-1, ERANGE, fn, "impossible atmospheric pressure: %g mbar", pressure);

  prop_error(fn, make_itrf_site(latitude, longitude, height, loc), 0);

  loc->temperature = temperature;
  loc->pressure = pressure;

  return 0;
}

/**
 * Initializes an observing site with the specified International Terrestrial Reference Frame
 * (ITRF) / GRS80 location, and sets mean (annual) weather parameters based on that location.
 *
 * For the highest precision (&mu;as level) applications you should make sure that the location
 * provided here and the Earth-orientation parameters (EOP) used (in setting `novas_timescale`
 * and in `novas_make_frame()`) are provided in the same ITRF realization. You can use
 * `novas_itrf_transform_eop()` to change the ITRF realization for the EOP values,
 * if necessary.
 *
 * @param latitude      [deg] Geodetic (ITRF / GRS80) latitude; north positive.
 * @param longitude     [deg] Geodetic (ITRF / GRS80) longitude; east positive.
 * @param height        [m] Geodetic (ITRF / GRS80) altitude above sea level of the observer.
 * @param[out] site     Pointer to the data structure to populate.
 *
 * @return              0 if successful, or -1 if the output argument is NULL (errno set to
 *                      EINVAL), or if the latitude is outside of the [-90:90] range (errno
 *                      set to ERANGE).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa make_gps_site(), make_xyz_site()
 * @sa make_itrf_observer(), make_observer_at_site(), novas_set_default_weather(),
 *     novas_itrf_transform_site(), novas_itrf_transform_eop()
 */
int make_itrf_site(double latitude, double longitude, double height, on_surface *site) {
  static const char *fn = "make_itrf_site";

  if(!site)
    return novas_error(-1, EINVAL, fn, "site is NULL");

  if(fabs(latitude) > 90.0)
    return novas_error(-1, ERANGE, fn, "latitude %g is outside of [-90:90] range.\n", latitude);

  site->latitude = latitude;
  site->longitude = longitude;
  site->height = height;

  novas_set_default_weather(site);
  return 0;
}

/**
 * Initializes an observing site with the specified GPS / WGS84 location, and sets mean (annual)
 * weather parameters based on that location.
 *
 * For the highest (&mu;as / mm level) precision, you probably should use an ITRF location
 * instead of a GPS based location.
 *
 * @param latitude      [deg] Geodetic (GPS / WGS84) latitude; north positive.
 * @param longitude     [deg] Geodetic (GPS / WGS84) longitude; east positive.
 * @param height        [m] Geodetic (GPS / WGS84) altitude above sea level of the observer.
 * @param[out] site     Pointer to the data structure to populate.
 *
 * @return              0 if successful, or -1 if the output argument is NULL (errno set to
 *                      EINVAL), or if the latitude is outside of the [-90:90] range (errno
 *                      set to ERANGE).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa make_gps_site(), make_xyz_site()
 * @sa make_itrf_observer(), make_observer_at_site(), novas_set_default_weather()
 */
int make_gps_site(double latitude, double longitude, double height, on_surface *site) {
  double xyz[3] = {0.0};

  novas_geodetic_to_cartesian(longitude, latitude, height, NOVAS_WGS84_ELLIPSOID, xyz);
  novas_cartesian_to_geodetic(xyz, NOVAS_GRS80_ELLIPSOID, &longitude, &latitude, &height);

  prop_error("make_gps_site", make_itrf_site(latitude, longitude, height, site), 0);
  return 0;
}

/**
 * Initializes an observing site with the specified Cartesian geocentric _xyz_ location, and sets
 * mean (annual) weather parameters based on that location.
 *
 * For the highest precision (&mu;as level) applications you should make sure that the site
 * coordinates and the Earth-orientation parameters (EOP) used (in setting `novas_timescale` and in
 * `novas_make_frame()`) are provided in the same ITRF realization. You can use
 * `novas_itrf_transform()` or `novas_itrf_transform_eop()` to change the ITRF realization for the
 * site coordinates and/or the EOP values, if necessary.
 *
 * @param xyz           [m] Cartesian geocentric position.
 * @param[out] site     Pointer to the data structure to populate.
 *
 * @return              0 if successful, or -1 if either of the arguments is NULL (errno set to
 *                      EINVAL), or if the latitude is outside of the [-90:90] range (errno
 *                      set to ERANGE).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa make_gps_site(), make_itrf_site(), make_xyz_site()
 * @sa make_observer_at_site(), novas_itrf_transform(), novas_itrf_transform_eop(),
 *     novas_set_default_weather()
 */
int make_xyz_site(const double *restrict xyz, on_surface *restrict site) {
  static const char *fn = "make_xyz_site";
  double lon = 0.0, lat = 0.0, alt = 0.0;

  if(!xyz)
    return novas_error(-1, EINVAL, fn, "input xzy position is NULL");

  prop_error(fn, novas_cartesian_to_geodetic(xyz, NOVAS_GRS80_ELLIPSOID, &lon, &lat, &alt), 0);
  prop_error(fn, make_itrf_site(lat, lon, alt, site), 0);
  return 0;
}

/**
 * Populates an 'in_space' data structure, for an observer situated on a near-Earth spacecraft,
 * with the provided position and velocity components. Both input vectors are assumed with respect
 * to true equator and equinox of date.
 *
 * @param sc_pos    [km] Geocentric (x, y, z) position vector. NULL defaults to the origin
 * @param sc_vel    [km/s] Geocentric (x, y, z) velocity vector. NULL defaults to zero speed.
 * @param[out] loc  Pointer to earth-orbit location data structure to populate.
 * @return          0 if successful, or -1 if the output argument is NULL.
 *
 * @sa make_observer_in_space(), IN_SPACE_INIT
 */
int make_in_space(const double *sc_pos, const double *sc_vel, in_space *loc) {
  if(!loc)
    return novas_error(-1, EINVAL, "make_in_space", "NULL output location pointer");

  if(sc_pos)
    memcpy(loc->sc_pos, sc_pos, sizeof(loc->sc_pos));
  else
    memset(loc->sc_pos, 0, sizeof(loc->sc_pos));

  if(sc_vel)
    memcpy(loc->sc_vel, sc_vel, sizeof(loc->sc_vel));
  else
    memset(loc->sc_vel, 0, sizeof(loc->sc_vel));

  return 0;
}

/**
 * Populates an 'observer' data structure for an observer moving relative to the surface of Earth,
 * such as an airborne observer. Airborne observers have an earth fixed momentary location,
 * defined by longitude, latitude, and altitude, the same was as for a stationary observer on
 * Earth, but are moving relative to the surface, such as in an aircraft or balloon observatory.
 *
 * @param location    Current geodetic location, e.g. as populated with `make_gps_site()` or
 *                    similar.
 * @param vel         [km/s] Surface velocity.
 * @param[out] obs    Pointer to data structure to populate.
 * @return            0 if successful, or -1 if the output argument is NULL.
 *
 *
 * @sa make_itrf_site(), make_gps_site(), make_xyz_site()
 * @sa make_gps_observer(), make_itrf_observer(), make_observer_at_site(), make_observer_in_space(),
 *     make_solar_system_observer(), make_observer_at geocenter(), novas_make_frame()
 *
 * @since 1.1
 * @author Attila Kovacs
 */
int make_airborne_observer(const on_surface *location, const double *vel, observer *obs) {
  in_space motion = IN_SPACE_INIT;

  if(!vel)
    return novas_error(-1, EINVAL, "make_airborne_observer", "NULL velocity");

  memcpy(motion.sc_vel, vel, sizeof(motion.sc_vel));

  prop_error("make_airborne_observer", make_observer(NOVAS_AIRBORNE_OBSERVER, location, &motion, obs), 0);
  return 0;
}

/**
 * Populates an 'observer' data structure, for an observer situated on a near-Earth spacecraft,
 * with the specified geocentric position and velocity vectors. Solar-system observers are similar
 * to observers in Earth-orbit but their momentary position and velocity is defined relative to
 * the Solar System Barycenter, instead of the geocenter.
 *
 * @param sc_pos        [AU] Solar-system barycentric (x, y, z) position vector in ICRS.
 * @param sc_vel        [AU/day] Solar-system barycentric (x, y, z) velocity vector in ICRS.
 * @param[out] obs      Pointer to the data structure to populate
 * @return              0 if successful, or -1 if the output argument is NULL.
 *
 * @sa make_gps_observer(), make_itrf_observer(), make_observer_at_site(),
 *     make_airborne_observer(), make_observer_in_space(), make_solar_system_observer()
 * @sa novas_make_frame()
 *
 * @since 1.1
 * @author Attila Kovacs
 */
int make_solar_system_observer(const double *sc_pos, const double *sc_vel, observer *obs) {
  static const char *fn = "make_observer_in_space";
  in_space loc;
  prop_error(fn, make_in_space(sc_pos, sc_vel, &loc), 0);
  prop_error(fn, make_observer(NOVAS_SOLAR_SYSTEM_OBSERVER, NULL, &loc, obs), 0);
  return 0;
}

/**
 * Corrects position vector for aberration of light.  Algorithm includes relativistic terms.
 *
 * NOTES:
 * <ol>
 * <li>This function is called by `place()` to account for aberration when calculating the
 * position of the source.</li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 * <li>Murray, C. A. (1981) Mon. Notices Royal Ast. Society 195, 639-648.</li>
 * <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 * </ol>
 *
 * @param pos         [AU]  Position vector of source relative to observer
 * @param vobs        [AU/day]  Velocity vector of observer, relative to the solar system
 *                    barycenter.
 * @param lighttime   [day] Light time from object to Earth (if known). Or set to 0, and this
 *                    function will compute it as needed.
 * @param[out] out    [AU] Position vector, referred to origin at center of mass of the Earth,
 *                    corrected for aberration. It can be the same vector as one of the inputs.
 *
 * @return            0 if successful, or -1 if any of the vector arguments are NULL.
 *
 * @sa frame_aberration()
 */
int aberration(const double *pos, const double *vobs, double lighttime, double *out) {
  double p1mag, vemag, beta, cosd, gammai, p, q, r;

  if(!pos || !vobs || !out)
    return novas_error(-1, EINVAL, "aberration", "NULL input or output 3-vector: pos=%p, vobs=%p, out=%p", pos, vobs, out);

  vemag = novas_vlen(vobs);
  if(!vemag) {
    if(out != pos)
      memcpy(out, pos, XYZ_VECTOR_SIZE);
    return 0;
  }

  beta = vemag / C_AUDAY;

  if(lighttime <= 0.0) {
    p1mag = novas_vlen(pos);
    lighttime = p1mag / C_AUDAY;
  }
  else
    p1mag = lighttime * C_AUDAY;

  cosd = novas_vdot(pos, vobs) / (p1mag * vemag);
  gammai = sqrt(1.0 - beta * beta);
  p = beta * cosd;
  q = (1.0 + p / (1.0 + gammai)) * lighttime;
  r = 1.0 + p;

  out[0] = (gammai * pos[0] + q * vobs[0]) / r;
  out[1] = (gammai * pos[1] + q * vobs[1]) / r;
  out[2] = (gammai * pos[2] + q * vobs[2]) / r;

  return 0;
}

/**
 * Calculates the ICRS position and velocity of the observer relative to the Solar System
 * Barycenter (SSB).
 *
 * @param jd_tdb        [day] Barycentric Dynamical Time (TDB) based Julian date.
 * @param ut1_to_tt     [s] TT - UT1 time difference. Used only when 'location->where' is
 *                      NOVAS_OBSERVER_ON_EARTH (1) or NOVAS_OBSERVER_IN_EARTH_ORBIT (2).
 * @param accuracy      NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param obs           The observer location, relative to which the output positions and
 *                      velocities are to be calculated
 * @param geo_pos       [AU] ICRS position vector of the geocenter w.r.t. the Solar System
 *                      Barycenter (SSB). If either geo_pos or geo_vel is NULL, it will be
 *                      calculated when needed.
 * @param geo_vel       [AU/day] ICRS velocity vector of the geocenter w.r.t. the Solar System
 *                      Barycenter (SSB). If either geo_pos or geo_vel is NULL, it will be
 *                      calculated when needed.
 * @param[out] pos      [AU] Position 3-vector of the observer w.r.t. the Solar System Barycenter
 *                      (SSB). It may be NULL if not required.
 * @param[out] vel      [AU/day] Velocity 3-vector of the observer w.r.t. the Solar System
 *                      Barycenter (SSB). It must be distinct from the pos output vector, and may
 *                      be NULL if not required.
 * @return              0 if successful, or the error from geo_posvel(), or else -1 (with errno
 *                      indicating the type of error).
 *
 * @author Attila Kovacs
 * @since 1.3
 *
 * @see novas_make_frame()
 */
int obs_posvel(double jd_tdb, double ut1_to_tt, enum novas_accuracy accuracy, const observer *restrict obs,
        const double *restrict geo_pos, const double *restrict geo_vel, double *restrict pos, double *restrict vel) {
  static const char *fn = "get_obs_posvel";

  if(!obs)
    return novas_error(-1, EINVAL, fn, "NULL observer parameter");

  if(obs->where < 0 || obs->where >= NOVAS_OBSERVER_PLACES)
    return novas_error(-1, EINVAL, fn, "Invalid observer location: %d", obs->where);

  if(pos == vel)
    return novas_error(-1, EINVAL, fn, "identical output pos and vel pointers @ %p.", pos);

  if(obs->where == NOVAS_SOLAR_SYSTEM_OBSERVER) {
    if(pos)
      memcpy(pos, obs->near_earth.sc_pos, XYZ_VECTOR_SIZE);
    if(vel)
      memcpy(vel, obs->near_earth.sc_vel, XYZ_VECTOR_SIZE);
    return 0;
  }

  if(!geo_pos || !geo_vel) {
    const double tdb2[2] = { jd_tdb };
    object earth = NOVAS_EARTH_INIT;
    double gpos[3], gvel[3];
    prop_error(fn, ephemeris(tdb2, &earth, NOVAS_BARYCENTER, accuracy, gpos, gvel), 0);
    if(pos)
      memcpy(pos, gpos, XYZ_VECTOR_SIZE);
    if(vel)
      memcpy(vel, gvel, XYZ_VECTOR_SIZE);
  }
  else {
    if(pos)
      memcpy(pos, geo_pos, XYZ_VECTOR_SIZE);
    if(vel)
      memcpy(vel, geo_vel, XYZ_VECTOR_SIZE);
  }

  // ---------------------------------------------------------------------
  // Get position and velocity of observer.
  // ---------------------------------------------------------------------
  switch(obs->where) {
    case NOVAS_OBSERVER_ON_EARTH:
    case NOVAS_AIRBORNE_OBSERVER:
    case NOVAS_OBSERVER_IN_EARTH_ORBIT: {
      double pog[3] = {0}, vog[3] = {0};
      int i;

      double jd_tt = jd_tdb - tt2tdb(jd_tdb) / DAY;

      prop_error(fn, geo_posvel(jd_tt, ut1_to_tt, accuracy, obs, pog, vog), 0);

      for(i = 3; --i >= 0;) {
        if(pos)
          pos[i] += pog[i];
        if(vel)
          vel[i] = novas_add_vel(vel[i], vog[i]);
      }

      break;
    }

    default:
      // Nothing to do
      ;
  }

  return 0;
}

/**
 * Moves the origin of coordinates from the barycenter of the solar system to the observer
 * (or the geocenter); i.e., this function accounts for parallax (annual+geocentric or just
 * annual).
 *
 * REFERENCES:
 * <ol>
 * <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 * </ol>
 *
 * @param pos             [AU] Position vector, referred to origin at solar system barycenter.
 * @param pos_obs         [AU] Position vector of observer (or the geocenter), with respect to
 *                        origin at solar system barycenter.
 * @param[out] out        [AU] Position vector, referred to origin at center of mass of the Earth.
 *                        It may be NULL if not required, or be the same vector as either of the
 *                        inputs.
 * @param[out] lighttime  [day] Light time from object to Earth. It may be NULL if not required.
 * @return                0 if successful, or -1 if any of the essential pointer arguments is
 *                        NULL.
 *
 * @sa novas_make_frame(), light_time2()
 */
int bary2obs(const double *pos, const double *pos_obs, double *out, double *restrict lighttime) {
  int j;

  // Default output value in case of error return
  if(lighttime)
    *lighttime = NAN;

  if(!pos || !pos_obs || !out)
    return novas_error(-1, EINVAL, "bary2obs", "NULL input or output 3-vector: pos=%p, pos_obs=%p, out=%p", pos, pos_obs, out);

  // Translate vector to geocentric coordinates.
  for(j = 3; --j >= 0;)
    out[j] = pos[j] - pos_obs[j];

  // Calculate length of vector in terms of light time.
  if(lighttime)
    *lighttime = novas_vlen(out) / C_AUDAY;

  return 0;
}

/**
 * Calculates the positions and velocities for the Solar-system bodies, e.g. for use for
 * gravitational deflection calculations. The planet positions are calculated relative to the
 * observer location, while velocities are w.r.t. the SSB. Both positions and velocities are
 * antedated for light travel time, so they accurately reflect the apparent position (and
 * barycentric motion) of the bodies from the observer's perspective.
 *
 *
 * @param jd_tdb        [day] Barycentric Dynamical Time (TDB) based Julian date
 * @param accuracy      NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1). In full accuracy
 *                      mode, it will calculate the deflection due to the Sun, Jupiter, Saturn and
 *                      Earth. In reduced accuracy mode, only the deflection due to the Sun is
 *                      calculated.
 * @param pos_obs       [AU] Position 3-vector of observer (or the geocenter), with respect to
 *                      origin at solar system barycenter, referred to ICRS axes.
 * @param pl_mask       Bitwise `(1 << planet-number)` mask indicating which planets to request
 *                      data for. See enum novas_planet for the enumeration of planet numbers.
 * @param[out] planets  Pointer to apparent planet data to populate. The planets with non-zero
 *                      mask bits will have have positions and velocities calculated. See enum
 *                      novas_planet for the enumeration of planet numbers.
 * @return              0 if successful, -1 if any of the pointer arguments is NULL or if the
 *                      output vector is the same as pos_obs, or the error from ephemeris().
 *
 * @sa enum novas_planet, grav_planets(), grav_undo_planets(), set_planet_provider(),
 *     set_planet_provider_hp()
 *
 * @since 1.1
 * @author Attila Kovacs
 */
int obs_planets(double jd_tdb, enum novas_accuracy accuracy, const double *restrict pos_obs, int pl_mask,
        novas_planet_bundle *restrict planets) {
  static const char *fn = "obs_planets";

  static object body[NOVAS_PLANETS];
  static int initialized;

  enum novas_debug_mode dbmode = novas_get_debug_mode();
  int i, error = 0;

  if(!planets)
    return novas_error(-1, EINVAL, fn, "NULL planet data");

  planets->mask = 0;

  if(!pos_obs)
    return novas_error(-1, EINVAL, fn, "NULL observer position parameter");

  // Set up the structures of type 'object' containing the body information.
  if(!initialized) {
    for(i = 0; i < NOVAS_PLANETS; i++)
      make_planet(i, &body[i]);
    initialized = 1;
  }

  // Temporarily disable debug traces, unless in extra debug mode
  if(dbmode != NOVAS_DEBUG_EXTRA)
    novas_debug(NOVAS_DEBUG_OFF);

  // Cycle through gravitating bodies.
  for(i = 0; i < NOVAS_PLANETS; i++) {
    const int bit = (1 << i);
    double tl;
    int stat;

    if((pl_mask & bit) == 0)
      continue;

    // Calculate positions and velocities antedated for light time.
    stat = light_time2(jd_tdb, &body[i], pos_obs, 0.0, accuracy, &planets->pos[i][0], &planets->vel[i][0], &tl);
    if(stat) {
      if(!error)
        error = stat > 10 ? stat - 10 : -1;
      continue;
    }

    planets->mask |= bit;
  }

  // Re-enable debug traces
  novas_debug(dbmode);

  // If could not calculate deflection due to Sun, return with error
  if((planets->mask & (1 << NOVAS_SUN)) == 0)
    prop_error("grav_init_planet:sun", error, 0);

  // If could not get positions for another gravitating body then
  // return error only if in extra debug mode...
  if(planets->mask != pl_mask && novas_get_debug_mode() == NOVAS_DEBUG_EXTRA)
    prop_error(fn, error, 0);

  return 0;
}

/**
 * Computes the geocentric position and velocity of a solar system body, as antedated for
 * light-time. It is effectively the same as the original NOVAS C light_time(), except that
 * this returns the antedated source velocity vector also.
 *
 * NOTES:
 * <ol>
 * <li>This function is called by `novas_geom_posvel()` or `place()` to calculate observed
 * positions, radial velocity, and distance for the time when the observed light originated
 * from the source.</li>
 * </ol>
 *
 * @param jd_tdb          [day] Barycentric Dynamical Time (TDB) based Julian date
 * @param body            Pointer to structure containing the designation for the solar system
 *                        body
 * @param pos_obs         [AU] Position 3-vector of observer (or the geocenter), with respect
 *                        to origin at solar system barycenter, referred to ICRS axes.
 * @param tlight0         [day] First approximation to light-time (can be set to 0.0 if not
 *                        readily avasilable -- so it will be calculated as needed).
 * @param accuracy        NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] p_src_obs  [AU] Position 3-vector of body, relative to observer, referred to ICRS
 *                        axes, components in AU.
 * @param[out] v_ssb      [AU/day] Velocity 3-vector of body, with respect to the Solar-system
 *                        barycenter, referred to ICRS axes.
 * @param[out] tlight     [day] Calculated light time, or NAN when returning with an error code.
 *
 * @return            0 if successful, -1 if any of the pointer arguments is NULL or if the
 *                    output vectors are the same or if they are the same as pos_obs, 1 if
 *                    the algorithm failed to converge after 10 iterations, or 10 + the error
 *                    from ephemeris().
 *
 * @sa light_time()
 * @sa novas_sky_pos()
 *
 * @since 1.0
 * @author Attila Kovacs
 */
int light_time2(double jd_tdb, const object *restrict body, const double *restrict pos_obs, double tlight0,
        enum novas_accuracy accuracy, double *p_src_obs, double *restrict v_ssb, double *restrict tlight) {
  static const char *fn = "light_time2";
  int iter = 0;

  double tol, jd[2] = {0};

  if(!tlight)
    return novas_error(-1, EINVAL, fn, "NULL 'tlight' output pointer");

  // Default return value.
  *tlight = NAN;

  if(!body || !pos_obs)
    return novas_error(-1, EINVAL, fn, "NULL input pointer: body=%p, pos_obs=%p", body, pos_obs);

  // Set light-time convergence tolerance.  If full-accuracy option has
  // been selected, split the Julian date into whole days + fraction of
  // day.
  if(accuracy == NOVAS_FULL_ACCURACY) {
    tol = 1.0e-12;

    jd[0] = floor(jd_tdb);
    jd[1] = jd_tdb - jd[0];
  }
  else {
    tol = 1.0e-9;

    jd[0] = jd_tdb;
  }

  // Iterate to obtain correct light-time (usually converges rapidly).
  for(iter = 0; iter < novas_inv_max_iter; iter++) {
    int error;
    double dt = 0.0;

    error = ephemeris(jd, body, NOVAS_BARYCENTER, accuracy, p_src_obs, v_ssb);
    bary2obs(p_src_obs, pos_obs, p_src_obs, tlight);
    prop_error(fn, error, 10);

    dt = *tlight - tlight0;
    if(fabs(dt) <= tol)
      return 0;

    jd[1] -= dt;
    tlight0 = *tlight;
  }

  return novas_error(1, ECANCELED, fn, "failed to converge");
}

/**
 * Computes the geocentric position of a solar system body, as antedated for light-time.
 *
 *
 * @param jd_tdb      [day] Barycentric Dynamical Time (TDB) based Julian date
 * @param body        Pointer to structure containing the designation for the solar system body
 * @param pos_obs     [AU] Position 3-vector of observer (or the geocenter), with respect to
 *                    origin at solar system barycenter, referred to ICRS axes.
 * @param tlight0     [day] First approximation to light-time (can be set to 0.0 if not readily
 *                    available -- it will then be computed as needed).
 * @param accuracy    NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] pos_src_obs    [AU] Position 3-vector of body, with respect to origin at observer
 *                            (or the geocenter), referred to ICRS axes. It can be the same vector
 *                            as either of the inputs.
 * @param[out] tlight [day] Calculated light time
 *
 * @return            0 if successful, -1 if any of the poiinter arguments is NULL, 1 if the
 *                    algorithm failed to converge after 10 iterations, or 10 + the error from
 *                    ephemeris().
 *
 * @sa light_time2(), novas_make_frame()
 */
short light_time(double jd_tdb, const object *restrict body, const double *pos_obs, double tlight0, enum novas_accuracy accuracy,
        double *pos_src_obs, double *restrict tlight) {
  double vel[3];
  prop_error("light_time", light_time2(jd_tdb, body, pos_obs, tlight0, accuracy, pos_src_obs, vel, tlight), 0);
  return 0;
}

/**
 * Converts a 3D line-of-sight vector (&delta;&phi;, &delta;&theta; &delta;r) to a rectangular
 * equatorial (&delta;x, &delta;y, &delta;z) vector.
 *
 * @param los         [arb.u.] Line-of-sight 3-vector (&delta;&phi;, &delta;&theta; &delta;r).
 * @param lon         [deg] Line-of-sight longitude.
 * @param lat         [deg] Line-of-sight latitude.
 * @param[out] xyz    [arb.u.] Output rectangular equatorial 3-vector (&delta;x, &delta;y, &delta;z),
 *                    in the same units as the input. It may be the same vector as the input.
 * @return            0 if successful, or else -1 if either vector argument is NULL (errno will be
 *                    set to EINVAL).
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_xyz_to_los(), novas_uvw_to_xyz()
 */
int novas_los_to_xyz(const double *los, double lon, double lat, double *xyz) {
  static const char *fn = "novas_los_to_xyz";

  double slon, clon, slat, clat;
  double dlon, dlat, dr, clatdr_m_slatdlat;

  if(!xyz)
    return novas_error(-1, EINVAL, fn, "output xyz vector is NULL");

  if(xyz != los)
    memset(xyz, 0, XYZ_VECTOR_SIZE);

  if(!los)
    return novas_error(-1, EINVAL, fn, "input los vector is NULL");

  lon *= DEGREE;
  lat *= DEGREE;

  slon = sin(lon);
  clon = cos(lon);
  slat = sin(lat);
  clat = cos(lat);

  dlon = los[0];
  dlat = los[1];
  dr = los[2];

  clatdr_m_slatdlat = clat * dr - slat * dlat;

  // Transform motion vector to equatorial system.
  xyz[0] = clon * clatdr_m_slatdlat - slon * dlon;
  xyz[1] = clon * dlon + slon * clatdr_m_slatdlat;
  xyz[2] = clat * dlat + slat * dr;

  return 0;
}

/**
 * Converts a 3D rectangular equatorial (&delta;x, &delta;y, &delta;z) vector to a polar
 * (&delta;&phi;, &delta;&theta; &delta;r) vector along a line-of-sight.
 *
 * @param xyz         [arb.u.] Rectangular equatorial 3-vector (&delta;x, &delta;y, &delta;z).
 * @param lon         [deg] Line-of-sight longitude.
 * @param lat         [deg] Line-of-sight latitude.
 * @param[out] los    [arb.u.] Output line-of-sight 3-vector (&delta;&phi;, &delta;&theta;
 *                    &delta;r), in the same units as the input. It may be the same vector as the
 *                    input.
 * @return            0 if successful, or else -1 if either vector argument is NULL (errno will be
 *                    set to EINVAL).
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_los_to_xyz(), novas_xyz_to_uvw()
 */
int novas_xyz_to_los(const double *xyz, double lon, double lat, double *los) {
  static const char *fn = "novas_xyz_to_los";

  double slon, clon, slat, clat;
  double x, y, z, clonx_slony;

  if(!los)
    return novas_error(-1, EINVAL, fn, "output los vector is NULL");

  if(los != xyz)
    memset(los, 0, XYZ_VECTOR_SIZE);

  if(!xyz)
    return novas_error(-1, EINVAL, fn, "input xyz vector is NULL");

  lon *= DEGREE;
  lat *= DEGREE;

  slon = sin(lon);
  clon = cos(lon);
  slat = sin(lat);
  clat = cos(lat);

  x = xyz[0];
  y = xyz[1];
  z = xyz[2];

  clonx_slony = clon * x + slon * y;

  // Transform motion vector to equatorial system.
  los[0] = clon * y - slon * x;
  los[1] = clat * z - slat * clonx_slony;
  los[2] = clat * clonx_slony + slat * z;

  return 0;
}

/**
 * Converts rectangular telescope x,y,z (absolute or relative) coordinates (in ITRS) to equatorial
 * u,v,w projected coordinates for a specified line of sight.
 *
 * x,y,z are Cartesian coordinates w.r.t the Greenwich meridian, in the ITRS frame. The directions
 * are x: long=0, lat=0; y: long=90, lat=0; z: lat=90.
 *
 * u,v,w are Cartesian coordinates (u,v) along the local equatorial R.A. and declination
 * directions as seen from a direction on the sky (w). As such, they are effectively ITRS-based
 * line-of-sight (LOS) coordinates.
 *
 * @param xyz           [arb.u.] Absolute or relative x,y,z coordinates (double[3]).
 * @param ha            [h] Hourangle (LST - RA) i.e., the difference between the Local (apparent)
 *                      Sidereal Time and the apparent (true-of-date) Right Ascension of observed
 *                      source.
 * @param dec           [deg] Apparent (true-of-date) declination of source
 * @param[out] uvw      [arb.u.] Converted u,v,w coordinates (double[3]) in same units as xyz.
 *                      It may be the same vector as the input.
 *
 * @return              0 if successful, or else -1 if either vector argument is NULL (errno will
 *                      be set to EINVAL)
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_uvw_to_xyz()
 */
int novas_xyz_to_uvw(const double *xyz, double ha, double dec, double *uvw) {
  prop_error("novas_xyz_to_uvw", novas_xyz_to_los(xyz, -15.0 * ha, dec, uvw), 0);
  return 0;
}

/**
 * Converts equatorial u,v,w projected (absolute or relative) coordinates to rectangular telescope
 * x,y,z coordinates (in ITRS) to for a specified line of sight.
 *
 * u,v,w are Cartesian coordinates (u,v) along the local equatorial R.A. and declination
 * directions as seen from a direction on the sky (w). As such, they are effectively ITRS-based
 * line-of-sight (LOS) coordinates.
 *
 * x,y,z are Cartesian coordinates w.r.t the Greenwich meridian in the ITRS frame. The directions
 * are x: long=0, lat=0; y: long=90, lat=0; z: lat=90.
 *
 * @param xyz           [arb.u.] Absolute or relative u,v,w coordinates (double[3]).
 * @param ha            [h] Hourangle (LST - RA) i.e., the difference between the Local (apparent)
 *                      Sidereal Time and the apparent (true-of-date) Right Ascension of observed
 *                      source.
 * @param dec           [deg] Apparent (true-of-date) declination of source
 * @param[out] uvw      [arb.u.] Converted x,y,z coordinates (double[3]) in the same unit as uvw.
 *                      It may be the same vector as the input.
 *
 * @return              0 if successful, or else -1 if either vector argument is NULL
 *                      (errno will be set to EINVAL)
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_xyz_to_uvw()
 */
int novas_uvw_to_xyz(const double *uvw, double ha, double dec, double *xyz) {
  prop_error("novas_uvw_to_xyz", novas_los_to_xyz(uvw, -15.0 * ha, dec, xyz), 0);
  return 0;
}

/**
 * Returns the horizontal Parallactic Angle (PA) calculated for a gorizontal Az/El location of the
 * sky. The PA is the angle between the local horizontal coordinate directions and the local
 * true-of-date equatorial coordinate directions at the given location. The polar wobble is not
 * included in the calculation.
 *
 * The Parallactic Angle is sometimes referrred to as the Vertical Position Angle (VPA). Both
 * define the same quantity.
 *
 * @param az    [deg] Azimuth angle
 * @param el    [deg] Elevation angle
 * @param lat   [deg] Geodetic latitude of observer
 * @return      [deg] Parallactic Angle (PA). I.e., the clockwise position angle of the
 *              declination direction w.r.t. the elevation axis in the horizontal system. Same as
 *              the the clockwise position angle of the elevation direction w.r.t. the declination
 *              axis in the equatorial system.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_epa(), novas_h2e_offset()
 */
double novas_hpa(double az, double el, double lat) {
  double s, c;

  lat *= DEGREE;
  az *= DEGREE;
  el *= DEGREE;

  s = sin(lat);
  c = cos(lat);

  return atan2(-c * sin(az), s * cos(el) - c * sin(el) * cos(az)) / DEGREE;
}

/**
 * Returns the equatorial Parallactic Angle (PA) calculated for an R.A./Dec location of the sky at
 * a given sidereal time. The PA is the angle between the local horizontal coordinate directions
 * and the local true-of-date equatorial coordinate directions, at the given location and time.
 * The polar wobble is not included in the calculation.
 *
 * The Parallactic Angle is sometimes referrred to as the Vertical Position Angle (VPA). Both
 * define the same quantity.
 *
 * @param ha      [h] Hour angle (LST - RA) i.e., the difference between the Local (apparent)
 *                Sidereal Time and the apparent (true-of-date) Right Ascension of observed
 *                source.
 * @param dec     [deg] Apparent (true-of-date) declination of observed source
 * @param lat     [deg] Geodetic latitude of observer
 * @return        [deg] Parallactic Angle (PA). I.e., the clockwise position angle of the
 *                elevation direction w.r.t. the declination axis in the equatorial system. Same
 *                as the clockwise position angle of the declination direction w.r.t. the
 *                elevation axis, in the horizontal system.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_hpa(), novas_e2h_offset()
 */
double novas_epa(double ha, double dec, double lat) {
  double coslat;

  ha *= HOURANGLE;
  lat *= DEGREE;
  dec *= DEGREE;

  coslat = cos(lat);
  return atan2(coslat * sin(ha), sin(lat) * cos(dec) - coslat * sin(dec) * cos(ha)) / DEGREE;
}

/**
 * Converts coordinate offsets, from the local horizontal system to local equatorial offsets.
 * Converting between local flat projections and spherical coordinates usually requires a WCS
 * projection.
 *
 * REFERENCES:
 * <ol>
 * <li>Calabretta, M.R., &amp; Greisen, E.W., (2002), Astronomy &amp; Astrophysics, 395, 1077-1122.</li>
 * </ol>
 *
 * @param daz         [arcsec] Projected offset position in the azimuth direction. The projected
 *                    offset between two azimuth positions at the same reference elevation is
 *                    &delta;Az = (Az2 - Az1) * cos(El<sub>0</sub>).
 * @param del         [arcsec] projected offset position in the elevation direction
 * @param pa          [deg] Parallactic Angle
 * @param[out] dra    [arcsec] Output offset position in the local true-of-date R.A. direction. It
 *                    can be a pointer to one of the input coordinates, or NULL if not required.
 * @param[out] ddec   [arcsec] Output offset position in the local true-of-date declination
 *                    direction. It can be a pointer to one of the input coordinates, or NULL if
 *                    not required.
 * @return            0
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_e2h_offset(), novas_hpa()
 */
int novas_h2e_offset(double daz, double del, double pa, double *restrict dra, double *restrict ddec) {
  double dx = daz, dy = del, c, s;

  pa *= DEGREE;
  c = cos(pa);
  s = sin(pa);

  if(dra)
    *dra =  s * dy - c * dx;
  if(ddec)
    *ddec = s * dx + c * dy;

  return 0;
}

/**
 * Converts coordinate offsets, from the local equatorial system to local horizontal offsets.
 * Converting between local flat projections and spherical coordinates usually requires a WCS
 * projection.
 *
 * REFERENCES:
 * <ol>
 * <li>Calabretta, M.R., &amp; Greisen, E.W., (2002), Astronomy &amp; Astrophysics, 395, 1077-1122.</li>
 * </ol>
 *
 * @param dra         [arcsec] Projected ffset position in the apparent true-of-date R.A.
 *                    direction. E.g. The projected offset between two RA coordinates at a same
 *                    reference declination, is &delta;RA = (RA2 - RA1) * cos(Dec<sub>0</sub>).
 * @param ddec        [arcsec] Projected offset position in the apparent true-of-date declination
 *                    direction.
 * @param pa          [deg] Parallactic Angle
 * @param[out] daz    [arcsec] Output offset position in the local azimuth direction. It can be a
 *                    pointer to one of the input coordinates, or NULL if not required.
 * @param[out] del    [arcsec] Output offset position in the local elevation direction. It can be a
 *                    pointer to one of the input coordinates, or NULL if not required.
 * @return            0
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_h2e_offset(), novas_epa()
 */
int novas_e2h_offset(double dra, double ddec, double pa, double *restrict daz, double *restrict del) {
  return novas_h2e_offset(dra, ddec, pa, daz, del);
}

/**
 * Sets default weather parameters based on an approximate global model to the mean annualized
 * temperatures, based on Feulner et al. (2013), and scaling relations with altitude (up to 12 km).
 *
 * Humidity is set to 70% at sea level (which is a typical value globally), and adjusted to
 * decrease with altitude linearly at a rate of 7.5% per km up to 8000 meters. Above that a
 * quadratic model is assumed, peaking at 45% at 14 km -- based on the measured distribution by
 * Mendez-Astudillo et al. (2021). Finally above 20.8 km, zero humidity is assumed.
 *
 * These parameters are all very approximate, but in the absence of measured data, they represent
 * a best guess default model of sorts.
 *
 * REFERENCES:
 * <ol>
 * <li>Feulner, G., Rahmstorf, S., Levermann, A., and Volkwardt, S. (2013), Journal of Climate 26, 7136</li>
 * <li>Mendez-Astudillo, J., et al. (2021), Urban Heat Island (UHI) Mitigation (pp.43-59),
 * DOI:10.1007/978-981-33-4050-3_3
 * </li>
 * </ol>
 *
 * @param[in, out] site       Site containing geodetic loation as input, and poulated with typical
 *                            mean weather parameters for the output.
 * @return                    0 if successful, or else -1 if the site is NULL (errno is set to
 *                            EINVAL).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa make_itrf_site(), make_gps_site(), make_xyz_site(), make_itrf_observer(), make_gps_observer(),
 *     NOVAS_STANDARD_ATMOSPHERE
 */
int novas_set_default_weather(on_surface *site) {
  double ct;

  if(!site)
    return novas_error(-1, EINVAL, "novas_set_default_weather", "site is NULL");

  // Pressure, based on altitudde...
  site->pressure = 1010.0 * exp(-site->height / PRESSURE_SCALE_HEIGHT);

  // A rough model of mean surface temperature based on Feulner et al. (2013)
  ct = site->latitude < ANTARCTIC_LATITUDE ? ANTARCTIC_ADIABATIC_LAPSE_RATE : MOIST_ADIABATIC_LAPSE_RATE;
  site->temperature = 47.0 * cos(site->latitude * DEGREE) - 20.0;
  site->temperature -= (site->height < 12000.0 ? site->height : 12000.0) * ct;

  // A simple model based on data from Mendez-Astudillo et al. (2021).
  if(site->height >= 20800.0)
    site->humidity = 0.0;
  else if(site->height > 8000.0) {
    double dh = (site->height - 14000.0) / 6000.0;
    site->humidity = 10.0 + 35.0 * (1.0 - dh * dh);
  }
  else site->humidity = 70.0 - 0.0075 * site->height;

  return 0;
}

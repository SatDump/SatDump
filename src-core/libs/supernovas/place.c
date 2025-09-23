/**
 * @file
 *
 * @date Created  on Mar 5, 2025
 * @author Attila Kovacs and G. Kaplan
 *
 *  Various convenience variants of the NOVAS C `place()` function. Using `place()` or one of its
 *  variants is disfavored in SuperNOVAS. Instead, the new frame-based approach is recommended,
 *  via `novas_sky_pos()` (for apparent positions, including aberration and gravitational
 *  deflection) or `novas_geom_posvel() (for geometric positions).
 *
 *  Some of the functions here are now deprecated, since the new frame-based approach, introduced
 *  in v1.2 offers more versatile and transparent ways of calculating the same quantities.
 *
 *  Nevertheless, if you insist on using these older methods, here's a primer on the ways the
 *  `place()` variants differ from one-another:
 *
 *  __reference system__: The coordinate reference system in which the output position is
 *  expressed. Often it is ICRS / GCRS, or True of Date (TOD), but occasionally something else.
 *
 *  __observer location__: If the observer is not one of the call parameters than a fictitious
 *  observer located at the geocenter is assumed (e.g. for `astro_star(), `astro_planet()` or
 *  `place_icrs()`).
 *
 *  __position type__: The positions calculated can be geometric or apparent. Both include
 *  light-time correction (for Solar-system sources), but geometric positions do not correct for
 *  the aberration due to observer movement, and gravitational deflections around the major
 *  Solar-system bodies. Apparent positions, however, do include such corrections. The contract
 *  of the `place()` function is that if the `coord_sys` call parameter is `NOVAS_ICRS`, then
 *  it calculates a geometric place, while for all other reference systems it returns an
 *  apparent place.
 *
 *  For example, the following place() variants may be used for a geocentric observer place and
 *  a sidereal (non-Solar-system) source, depending on the reference system and the type of position
 *  to be calculated:
 *
 *  | system \ type     | geocentric / geometric | geocentric / apparent | topocentric / apparent |
 *  |:----------------- | ---------------------- | --------------------- | ---------------------- |
 *  | ICRS / GCRS       | `astro_star()`         | `virtual_star()`      | `local_star()`         |
 *  | TOD               |                        | `app_star()`          | `topo_star()`          |
 *
 * For Solar-system sources you have a matching set of functions but with `planet` in their name
 * instead of `star`, e.g. `astro_planet()` instead of `astro_star()`.
 *
 * And for geocentric observer locations, the functions below can be used to support further
 * reference systems and can be used with both Solar-system and sidereal sources:
 *
 *  | system \ type     | geocentric / geometric | geocentric / apparent |
 *  |:----------------- | ---------------------- | --------------------- |
 *  | ICRS / GCRS       | `place_icrs()`         | `place_gcrs()`        |
 *  | J2000             |                        | `place_j2000()`       |
 *  | MOD               |                        | `place_mod()`         |
 *  | TOD               |                        | `place_tod()`         |
 *  | CIRS              |                        | `place_cirs()`        |
 *
 * As you can see it's a little scattered and the nomenclature is not always telling either. Thus,
 * the preferred way is to use a universal recipe and specify the (a) reference system; (b)
 * observer location; and (c) whether to calculate geometric or apparent positions explcitly. Even
 * if it means a few more lines of code. In the new frame based approach this boils down to the
 * following steps:
 *
 *  1. Specify the observer location, (ground-based, airborne, on Earth-orbit, geocentric
 *  or heliocentric orbit). See @ref observer for options.
 *
 *  2. Create an observing frame for the type of observer and time of observation using
 *  `novas_make_frame()`.
 *
 *  3. Calculate geometric positions using `novas_geom_posvel()`, or apparent positions
 *  using `novas_sky_pos()`. Set the reference system as one of the call parameters, e.g.
 *  `NOVAS_ICRS` for ICRS, `NOVAS_TOD` for TOD etc.
 *
 *  4. If needed convert between position 3-vectors and RA/Dec coordinates using
 *  `vector2radec()` / `radec2vector()` as necessary.
 *
 *  @sa target.c, observer.c, timescale.c, frames.c
 */

#include <string.h>
#include <errno.h>

/// \cond PRIVATE
#define __NOVAS_INTERNAL_API__    ///< Use definitions meant for internal use by SuperNOVAS only
#include "novas.h"
/// \endcond

/**
 * Retrieves the position and velocity of a solar system body from a fundamental ephemeris.
 *
 * It is recommended that the input structure 'cel_obj' be created using make_object()
 *
 * @param jd_tdb    [day] Barycentric Dynamic Time (TDB) based Julian date
 * @param body      Pointer to structure containing the designation of the body of interest
 * @param origin    NOVAS_BARYCENTER (0) or NOVAS_HELIOCENTER (1)
 * @param accuracy  NOCAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] pos  [AU] Pointer to structure containing the designation of the body of interest
 * @param[out] vel  [AU/day] Velocity vector of the body at 'jd_tdb'; equatorial rectangular
 *                  coordinates in AU/day referred to the ICRS.
 * @return          0 if successful, -1 if the 'jd_tdb' or input object argument is NULL, or
 *                  else 1 if 'origin' is invalid, 2 if <code>cel_obj->type</code> is invalid,
 *                  10 + the error code from solarsystem(), or 20 + the error code from
 *                  readeph().
 *
 * @sa set_planet_provider(), set_planet_provider_hp(), set_ephem_provider()
 * @sa make_planet(), make_ephem_object()
 */
short ephemeris(const double *restrict jd_tdb, const object *restrict body, enum novas_origin origin,
        enum novas_accuracy accuracy, double *restrict pos, double *restrict vel) {
  static const char *fn = "ephemeris";

  double posvel[6] = {0};
  int error = 0;

  if(!jd_tdb || !body)
    return novas_error(-1, EINVAL, fn, "NULL input pointer: jd_tdb=%p, body=%p", jd_tdb, body);

  if(!pos || !vel)
    return novas_error(-1, EINVAL, fn, "NULL output pointer: pos=%p, vel=%p", pos, vel);

  // Check the value of 'origin'.
  if(origin < 0 || origin >= NOVAS_ORIGIN_TYPES)
    return novas_error(1, EINVAL, fn, "invalid origin type: %d", origin);

  // Invoke the appropriate ephemeris access software depending upon the
  // type of object
  switch(body->type) {

    case NOVAS_PLANET:
      // Get the position and velocity of a major planet, Pluto, Sun, or Moon.
      // When high accuracy is specified, use function 'solarsystem_hp' rather
      // than 'solarsystem'.

      if(accuracy == NOVAS_FULL_ACCURACY)
        error = planet_call_hp(jd_tdb, body->number, origin, pos, vel);
      else
        error = planet_call(jd_tdb[0] + jd_tdb[1], body->number, origin, pos, vel);

      prop_error("ephemeris:planet", error, 10);
      break;

    case NOVAS_EPHEM_OBJECT: {
      enum novas_origin eph_origin = NOVAS_HELIOCENTER;
      novas_ephem_provider ephem_call = get_ephem_provider();

      if(ephem_call) {
        // If there is a newstyle epehemeris access routine set, we will prefer it.
        error = ephem_call(body->name, body->number, jd_tdb[0], jd_tdb[1], &eph_origin, posvel, &posvel[3]);
      }
      else {
#  ifdef DEFAULT_READEPH
        // Use whatever readeph() was compiled or the equivalent user-defined call
        double *res = readeph(body->number, body->name, jd_tdb[0] + jd_tdb[1], &error);
        if(res == NULL) {
          error = 3;
          errno = ENOSYS;
        }
        else {
          memcpy(posvel, res, sizeof(posvel));
          free(res);
        }
#  else
        return novas_error(-1, errno, "ephemeris:ephem_object", "No ephemeris provider was defined. Call set_ephem_provider() prior.");
#  endif
      }

      prop_error("ephemeris:ephem_object", error, 20);

      // Check and adjust the origins as necessary.
      if(origin != eph_origin) {
        double pos0[3] = {0}, vel0[3] = {0};
        enum novas_planet refnum = (origin == NOVAS_BARYCENTER) ? NOVAS_SSB : NOVAS_SUN;
        int i;

        prop_error("ephemeris:origin", planet_call(jd_tdb[0] + jd_tdb[1], refnum, eph_origin, pos0, vel0), 10);

        for(i = 3; --i >= 0;) {
          posvel[i] -= pos[i];
          posvel[i + 3] = novas_add_vel(posvel[i + 3], vel[i]);
        }
      }

      // Break up 'posvel' into separate position and velocity vectors.
      memcpy(pos, posvel, XYZ_VECTOR_SIZE);
      memcpy(vel, &posvel[3], XYZ_VECTOR_SIZE);

      break;
    }

    case NOVAS_ORBITAL_OBJECT: {
      object center;
      double pos0[3] = {0}, vel0[3] = {0};
      int i;

      prop_error(fn, make_planet(body->orbit.system.center, &center), 0);
      prop_error(fn, ephemeris(jd_tdb, &center, origin, accuracy, pos0, vel0), 0);
      prop_error(fn, novas_orbit_posvel(jd_tdb[0] + jd_tdb[1], &body->orbit, accuracy, pos, vel), 0);

      for(i = 3; --i >= 0; ) {
        pos[i] += pos0[i];
        vel[i] += vel0[i];
      }

      break;
    }

    default:
      return novas_error(2, EINVAL, fn, "invalid Solar-system body type: %d", body->type);
  }

  return 0;
}

/**
 * Computes the apparent direction of a celestial object at a specified time and in a specified
 * coordinate system and for a given observer location.
 *
 * While <code>coord_sys</code> defines the coordinate referfence system, it is
 * <code>location->where</code> sets the origin of the reference place relative to which positions
 * and velocities are reported.
 *
 * For all but ICRS coordinate outputs, the calculated positions and velocities include aberration
 * corrections for the moving frame of the observer as well as gravitational deflection due to the
 * Sun and Earth and other major gravitating bodies in the Solar system, provided planet positions
 * are available via a novas_planet_provider function.
 *
 * In case of a dynamical equatorial system (such as CIRS or TOD) and an Earth-based observer, the
 * polar wobble parameters set via a prior call to cel_pole() together with he ut1_to_tt argument
 * decide whether the resulting 'topocentric' output frame is Pseudo Earth Fixed (PEF; if
 * cel_pole() was not set and DUT1 is 0) or ITRS (actual rotating Earth; if cel_pole() was set
 * and ut1_to_tt includes the DUT1 component).
 *
 * NOTES:
 * <ol>
 * <li>This version fixes a NOVAS C 3.1 issue that velocities and solar-system distances were not
 * antedated for light-travel time.</li>
 * <li>In a departure from the original NOVAS C, the radial velocity for major planets (and Sun and
 * Moon) includes gravitational redshift corrections for light originating at the surface, assuming
 * it's observed from near Earth or else from a large distance away.</li>
 * <li>As of SuperNOVAS v1.3, the returned radial velocity component is a proper observer-based
 * spectroscopic measure. In prior releases, and in NOVAS C 3.1, this was inconsistent, with
 * pseudo LSR-based measures being returned for catalog sources.</li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 *     <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 *     <li>Klioner, S. (2003), Astronomical Journal 125, 1580-1597.</li>
 * </ol>
 *
 * @param jd_tt         [day] Terrestrial Time (TT) based Julian date.
 * @param source        Pointer to a celestrial object data structure. Catalog objects musy have
 *                      ICRS coordinates. You can use `transform_cat()` to convert other catalog
 *                      systems to ICRS as necessary.
 * @param location      The observer location, relative to which the output positions and velocities
 *                      are to be calculated
 * @param ut1_to_tt     [s] TT - UT1 time difference. Used only when 'location->where' is
 *                      NOVAS_OBSERVER_ON_EARTH (1) or NOVAS_OBSERVER_IN_EARTH_ORBIT (2).
 * @param coord_sys     The coordinate system that defines the orientation of the celestial pole.
 *                      If it is NOVAS_ICRS (3), a geometric position and radial velocity is returned. For
 *                      all other systems, the returned position is the apparent position including
 *                      aberration and gravitational deflection corrections, and the radial velocity
 *                      is in the direction the eflected light was emitted from the source.
 * @param accuracy      NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] output   Data structure to populate with the result.
 * @return              0 if successful,<br>
 *                      1 if 'coord_sys' is invalid,<br>
 *                      2 if 'accuracy' is invalid,<br>
 *                      3 if the observer is at or very near (within ~1.5m of) the observed location,<br>
 *                      10--40: error is 10 + the error ephemeris(),<br>
 *                      40--50: error is 40 + the error from geo_posvel(),<br>
 *                      50--70: error is 50 + error from light_time2(),<br>
 *                      70--80: error is 70 + error from grav_def(),<br>
 *                      80--90: error is 80 + error from cio_location(),<br>
 *                      90--100: error is 90 + error from cio_basis().
 *
 * @sa place_star(), place_icrs(), place_gcrs(), place_cirs(), radec_star(), radec_planet()
 * @sa novas_sky_pos(), novas_geom_posvel(), get_ut1_to_tt()
 */
short place(double jd_tt, const object *restrict source, const observer *restrict location, double ut1_to_tt, enum novas_reference_system coord_sys,
        enum novas_accuracy accuracy, sky_pos *restrict output) {
  static const char *fn = "place";

  static THREAD_LOCAL enum novas_accuracy acc_last = -1;
  static THREAD_LOCAL double tlast = NAN;
  static THREAD_LOCAL double peb[3], veb[3], psb[3];

  observer obs;
  novas_planet_bundle planets = {0};
  int pl_mask = (accuracy == NOVAS_FULL_ACCURACY) ? grav_bodies_full_accuracy : grav_bodies_reduced_accuracy;
  double x, jd_tdb, pob[3], vob[3], pos[3] = {0.0}, vel[3], vpos[3], t_light, d_sb;
  int i;

  if(!source)
    return novas_error(-1, EINVAL, fn, "NULL input 'source' pointer");

  // Check for invalid value of 'coord_sys' or 'accuracy'.
  if(coord_sys < 0 || coord_sys >= NOVAS_ITRS)
    return novas_error(1, EINVAL, fn, "invalid reference system: %d", coord_sys);

  if(accuracy != NOVAS_FULL_ACCURACY && accuracy != NOVAS_REDUCED_ACCURACY)
    return novas_error(2, EINVAL, fn, "invalid accuracy: %d", accuracy);

  if(!location)
    make_observer_at_geocenter(&obs);
  else
    obs = *location;

  // Compute 'jd_tdb', the TDB Julian date corresponding to 'jd_tt'.
  jd_tdb = jd_tt + tt2tdb(jd_tt) / DAY;

  // ---------------------------------------------------------------------
  // Get position and velocity of Earth (geocenter) and Sun.
  // ---------------------------------------------------------------------
  if(!novas_time_equals_hp(jd_tt, tlast) || accuracy != acc_last) {
    static object earth = NOVAS_EARTH_INIT, sun = NOVAS_SUN_INIT;
    double vsb[3];
    const double tdb[2] = { jd_tdb };

    // Get position and velocity of Earth wrt barycenter of solar system, in ICRS.
    prop_error("place:ephemeris:earth", ephemeris(tdb, &earth, NOVAS_BARYCENTER, accuracy, peb, veb), 10);

    // Get position and velocity of Sun wrt barycenter of solar system, in ICRS.
    prop_error("place:ephemeris:sun", ephemeris(tdb, &sun, NOVAS_BARYCENTER, accuracy, psb, vsb), 10);

    tlast = jd_tt;
    acc_last = accuracy;
  }

  // ---------------------------------------------------------------------
  // Get position and velocity of observer.
  // ---------------------------------------------------------------------
  prop_error(fn, obs_posvel(jd_tdb, ut1_to_tt, accuracy, &obs, peb, veb, pob, vob), 40);

  // ---------------------------------------------------------------------
  // Find geometric position of observed object (ICRS)
  // ---------------------------------------------------------------------
  if(source->type == NOVAS_CATALOG_OBJECT) {
    // Observed object is star.
    double dt = 0.0;

    // Get position of star updated for its space motion.
    // (The motion calculated here is not used for radial velocity in `rad_vel2()`)
    starvectors(&source->star, pos, vel);

    dt = d_light(pos, pob);
    proper_motion(JD_J2000, pos, vel, (jd_tdb + dt), pos);

    // Get position of star wrt observer (corrected for parallax).
    bary2obs(pos, pob, pos, &t_light);

    output->dis = 0.0;
    d_sb = novas_vlen(pos);
  }
  else {
    // Get position of body wrt observer, antedated for light-time.
    prop_error(fn, light_time2(jd_tdb, source, pob, 0.0, accuracy, pos, vel, &t_light), 50);

    if(novas_vlen(pos) < 1e-11)
      return novas_error(3, EINVAL, fn, "observer is at or very near the observed location");

    // Calculate distance to Sun.
    d_sb = 0.0;
    for(i = 3; --i >= 0;) {
      double d = psb[i] - (pob[i] + pos[i]);
      d_sb += d * d;
    }
    d_sb = sqrt(d_sb);

    // AK: Fix for antedating distance and velocities...
    output->dis = t_light * C_AUDAY;
  }

  if(coord_sys != NOVAS_ICRS) {
    prop_error(fn, obs_planets(jd_tdb, accuracy, pob, pl_mask, &planets), 70);
  }

  // ---------------------------------------------------------------------
  // Compute direction in which light was emitted from the source
  // ---------------------------------------------------------------------
  if(coord_sys == NOVAS_ICRS || source->type == NOVAS_CATALOG_OBJECT) {
    // For sidereal sources and ICRS the 'velocity' position is the same as the geometric position.
    memcpy(vpos, pos, sizeof(pos));
  }
  else {
    double psrc[3];  // Barycentric position of Solar-systemn source (antedated)

    // A.K.: For this we calculate gravitational deflection of the observer seen from the source
    // i.e., reverse tracing the light to find the direction in which it was emitted.
    for(i = 3; --i >= 0;) {
      vpos[i] = -pos[i];
      psrc[i] = pos[i] + pob[i];
    }

    // vpos -> deflected direction in which observer is seen from source.
    prop_error(fn, grav_planets(vpos, psrc, &planets, vpos), 70);

    // vpos -> direction in which light was emitted from observer's perspective...
    for(i = 3; --i >= 0;)
      vpos[i] = -vpos[i];
  }

  // ---------------------------------------------------------------------
  // Compute radial velocity (all vectors in ICRS).
  // ---------------------------------------------------------------------
  output->rv = rad_vel2(source, vpos, vel, pos, vob, novas_vdist(pob, peb), novas_vdist(pob, psb), d_sb);

  if(coord_sys != NOVAS_ICRS) {
    // ---------------------------------------------------------------------
    // Apply gravitational deflection
    // ---------------------------------------------------------------------
    prop_error(fn, grav_planets(pos, pob, &planets, pos), 70);

    // ---------------------------------------------------------------------
    // Apply aberration correction.
    // ---------------------------------------------------------------------
    aberration(pos, vob, t_light, pos);
  }

  // ---------------------------------------------------------------------
  // Transform, if necessary, to output coordinate system.
  // ---------------------------------------------------------------------
  switch(coord_sys) {
    case NOVAS_J2000: {
      // Transform to equator and equinox of date.
      gcrs_to_j2000(pos, pos);
      break;
    }

    case NOVAS_MOD: {
      // Transform to equator and equinox of date.
      gcrs_to_mod(jd_tdb, pos, pos);
      break;
    }

    case NOVAS_TOD: {
      // Transform to equator and equinox of date.
      gcrs_to_tod(jd_tdb, accuracy, pos, pos);
      break;
    }

    case NOVAS_CIRS:
    case NOVAS_TIRS: {
      // Transform to equator and CIO of date.
      prop_error(fn, gcrs_to_cirs(jd_tdb, accuracy, pos, pos), 80);
      if(coord_sys == NOVAS_TIRS)
        spin(era(jd_tt, -ut1_to_tt / DAY), pos, pos);
      break;
    }

    default:
      // Nothing else to do.
      ;
  }

  // ---------------------------------------------------------------------
  // Finish up.
  // ---------------------------------------------------------------------
  vector2radec(pos, &output->ra, &output->dec);

  x = 1.0 / novas_vlen(pos);
  for(i = 3; --i >= 0;)
    output->r_hat[i] = pos[i] * x;

  return 0;
}

/**
 * Computes the apparent place of a star at the specified date, given its catalog mean place, proper
 * motion, parallax, and radial velocity. See `place()` for more information.
 *
 * It is effectively the same as calling place(), except for the `cat_entry` type target argument.
 *
 * For all but ICRS coordinate outputs, the calculated positions and velocities include aberration
 * corrections for the moving frame of the observer as well as gravitational deflection due to the
 * Sun and Earth and other major gravitating bodies in the Solar system, provided planet positions
 * are available via a novas_planet_provider function.
 *
 * REFERENCES:
 * <ol>
 *     <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 *     <li>Explanatory Supplement to the Astronomical Almanac (1992),Chapter 3.</li>
 * </ol>
 *
 * @param jd_tt     [day] Terrestrial Time (TT) based Julian date.
 * @param star      Pointer to catalog entry structure containing catalog data for the object in
 *                  the ICRS.
 * @param obs       Observer location (NULL defaults to geocentric)
 * @param ut1_to_tt [s] Difference TT-UT1 at 'jd_tt', in seconds of time.
 * @param system    The type of coordinate reference system in which coordinates are to be
 *                  returned.
 * @param accuracy  NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] pos  The position and radial velocity of of the catalog source in the specified
 *                  coordinate system and relative to the specified observer location (if
 *                  applicable)
 * @return          0 if successful, or -1 if one of the required arguments is NULL, or else 1 if
 *                  the observer location is invalid, or an error code from place().
 *
 * @author Attila Kovacs
 * @since 1.0
 *
 * @sa novas_sky_pos(), novas_geom_posvel(), get_ut1_to_tt()
 */
int place_star(double jd_tt, const cat_entry *restrict star, const observer *restrict obs, double ut1_to_tt,
        enum novas_reference_system system, enum novas_accuracy accuracy, sky_pos *restrict pos) {
  static const char *fn = "place_star";
  object source = {0};

  if(!star || !pos)
    return novas_error(-1, EINVAL, fn, "NULL input star=%p or output pos=%p pointer", star, pos);

  source.type = NOVAS_CATALOG_OBJECT;
  source.star = *star;

  prop_error(fn, place(jd_tt, &source, obs, ut1_to_tt, system, accuracy, pos), 0);
  return 0;
}

/**
 * Computes the place of a star at date 'jd_tt', for an observer in the specified coordinate
 * system, given the star's ICRS catalog place, proper motion, parallax, and radial velocity.
 *
 * Notwithstanding the different set of return values, this is the same as calling place_star()
 * with the same arguments.
 *
 * For all but ICRS coordinate outputs, the calculated positions and velocities include aberration
 * corrections for the moving frame of the observer as well as gravitational deflection due to the
 * Sun and Earth and other major gravitating bodies in the Solar system, provided planet positions
 * are available via a novas_planet_provider function.
 *
 * REFERENCES:
 * <ol>
 * <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 * <li>Explanatory Supplement to the Astronomical Almanac (1992), Chapter 3.</li>
 * </ol>
 *
 * @param jd_tt     [day] Terrestrial Time (TT) based Julian date.
 * @param star      Pointer to catalog entry structure containing catalog data for the object
 *                  in the ICRS.
 * @param obs       Observer location. It may be NULL if not relevant.
 * @param ut1_to_tt [s] Difference TT-UT1 at 'jd_tt', in seconds of time.
 * @param sys       Coordinate reference system in which to produce output values
 * @param accuracy  NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] ra   [h] Topocentric right ascension in hours, referred to true equator and equinox
 *                  of date 'jd_tt' or NAN when returning with an error code. (It may be NULL if
 *                  not required)
 * @param[out] dec  [deg] Topocentric declination in degrees, referred to true equator and equinox
 *                  of date 'jd_tt' or NAN when returning with an error code. (It may be NULL if
 *                  not required)
 * @param[out] rv   [AU/day] radial velocity relative ot observer, or NAN when returning with an
 *                  error code. (It may be NULL if not required)
 * @return          0 if successful, -1 if a required pointer argument is NULL, or else 20 + the
 *                  error from place_star().
 *
 * @since 1.0
 * @author Attila Kovacs
 *
 * @sa radec_planet()
 * @sa novas_sky_pos(), novas_geom_posvel()
 */
int radec_star(double jd_tt, const cat_entry *restrict star, const observer *restrict obs, double ut1_to_tt,
        enum novas_reference_system sys, enum novas_accuracy accuracy, double *restrict ra, double *restrict dec,
        double *restrict rv) {
  sky_pos output = SKY_POS_INIT;

  // Default return values in case of error.
  if(ra)
    *ra = NAN;
  if(dec)
    *dec = NAN;
  if(rv)
    *rv = NAN;

  prop_error("radec_star", place_star(jd_tt, star, obs, ut1_to_tt, sys, accuracy, &output), 20);

  if(ra)
    *ra = output.ra;
  if(dec)
    *dec = output.dec;
  if(rv)
    *rv = output.rv;

  return 0;
}

/**
 * Computes the place of a solar system body at the specified time for an observer in the
 * specified coordinate system. This is the same as calling place() with the same arguments,
 * except the different set of return values used.
 *
 * Notwithstanding the different set of return values, this is the same as calling place_planet()
 * with the same arguments.
 *
 * For all but ICRS coordinate outputs, the calculated positions and velocities include aberration
 * corrections for the moving frame of the observer as well as gravitational deflection due to the
 * Sun and Earth and other major gravitating bodies in the Solar system, provided planet positions
 * are available via a novas_planet_provider function.
 *
 * REFERENCES:
 * <ol>
 *     <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 *     <li>Explanatory Supplement to the Astronomical Almanac (1992),Chapter 3.</li>
 * </ol>
 *
 * @param jd_tt     [day] Terretrial Time (TT) based Julian date.
 * @param ss_body   Pointer to structure containing the body designation for the solar
 *                  system body.
 * @param obs       Observer location. It may be NULL if not relevant.
 * @param ut1_to_tt [s] Difference TT-UT1 at 'jd_tt', in seconds of time.
 * @param sys       Coordinate reference system in which to produce output values

 * @param accuracy  NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] ra   [h] Topocentric apparent right ascension in hours, referred to the
 *                  true equator and equinox of date, or NAN when returning with an error
 *                  code. (It may be NULL if not required)
 * @param[out] dec  [deg] Topocentric apparent declination in degrees referred to the
 *                  true equator and equinox of date, or NAN when returning with an error
 *                  code. (It may be NULL if not required)
 * @param[out] dis  [AU] True distance from Earth to the body at 'jd_tt' in AU, or NAN when
 *                  returning with an error code. (It may be NULL if not needed).
 * @param[out] rv   [AU/day] radial velocity relative ot observer, or NAN when returning with
 *                  an error code. (It may be NULL if not required)
 * @return          0 if successful, or -1 if the object argument is NULL or if
 *                  the value of 'where' in structure 'location' is invalid, or 10 + the
 *                  error code from place().
 *
 * @since 1.0
 * @author Attila Kovacs
 *
 * @sa radec_star()
 * @sa novas_sky_pos(), novas_geom_posvel()
 */
int radec_planet(double jd_tt, const object *restrict ss_body, const observer *restrict obs, double ut1_to_tt,
        enum novas_reference_system sys, enum novas_accuracy accuracy, double *restrict ra, double *restrict dec,
        double *restrict dis, double *restrict rv) {
  static const char *fn = "radec_planet";
  sky_pos output = SKY_POS_INIT;

  // Default return values in case of error.
  if(ra)
    *ra = NAN;
  if(dec)
    *dec = NAN;
  if(dis)
    *dis = NAN;
  if(rv)
    *rv = NAN;

  switch(ss_body->type) {
    case NOVAS_PLANET:
    case NOVAS_EPHEM_OBJECT:
    case NOVAS_ORBITAL_OBJECT:
      break;
    default:
      return novas_error(-1, EINVAL, fn, "object is not solar-system type: type=%d", ss_body->type);
  }

  prop_error(fn, place(jd_tt, ss_body, obs, ut1_to_tt, sys, accuracy, &output), 10);

  if(ra)
    *ra = output.ra;
  if(dec)
    *dec = output.dec;
  if(dis)
    *dis = output.dis;
  if(rv)
    *rv = output.rv;

  return 0;
}

/**
 * __reference_system__: TOD <br>
 * __observer location__: geocenter <br>
 * __position_type__: apparent <br>
 *
 * Computes the True-of-Date (TOD) apparent place of a star, as would be seen by an observer at
 * the geocenter, referenced to the dynamical equator of date, given its catalog mean place,
 * proper motion, parallax, and radial velocity.
 *
 * Notwithstanding the different set of return values, this is the same as calling place_star()
 * with a NULL observer location and NOVAS_TOD as the system for an object that specifies the
 * star.
 *
 * The calculated positions and velocities include aberration corrections for the moving frame of
 * the observer as well as gravitational deflection due to the Sun and Earth and other major
 * gravitating bodies in the Solar system, provided planet positions are available via a
 * novas_planet_provider function.
 *
 * REFERENCES:
 * <ol>
 *     <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 *     <li>Explanatory Supplement to the Astronomical Almanac (1992), Chapter 3.</li>
 * </ol>
 *
 * @param jd_tt     [day] Terretrial Time (TT) based Julian date.
 * @param star      Pointer to catalog entry structure containing catalog data for the object in
 *                  the ICRS.
 * @param accuracy  NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] ra   [h] Apparent (TOD) right ascension for a fictitous geocentric observer,
 *                  referred to true equator and equinox of date 'jd_tt' (it may be NULL if not
 *                  required).
 * @param[out] dec  [deg] Apparent (TOD) declination in degrees for a fictitous geocentric
 *                  observer, referred to true equator and equinox of date 'jd_tt' (it may be NULL
 *                  if not required).
 * @return          0 if successful, -1 if a required pointer argument is NULL, or else an
 *                  the error from make_object(), or 20 + the error from place().
 *
 * @sa place_tod(), novas_sky_pos(), place_star(), astro_star(), local_star(), topo_star(),
 *     virtual_star(), app_planet()
 * @sa novas_sky_pos(), make_observer_at_geocenter(), NOVAS_TOD
 */
short app_star(double jd_tt, const cat_entry *restrict star, enum novas_accuracy accuracy,
        double *restrict ra, double *restrict dec) {
  prop_error("app_star", radec_star(jd_tt, star, NULL, 0.0, NOVAS_TOD, accuracy, ra, dec, NULL), 0);
  return 0;
}

/**
 * __reference_system__: ICRS / GCRS <br>
 * __observer location__: geocenter <br>
 * __position_type__: apparent <br>
 *
 * Computes the virtual place of a star, as would be seen by an observer at the geocenter,
 * referenced to GCRS, at date 'jd_tt', given its catalog mean place, proper motion, parallax,
 * and radial velocity.
 *
 * Notwithstanding the different set of return values, this is the same as calling place_star()
 * with a NULL observer location and NOVAS_GCRS as the system, or place_gcrs() for an object that
 * specifies the star.
 *
 * The calculated positions and velocities include aberration corrections for the moving frame of
 * the observer as well as gravitational deflection due to the Sun and Earth and other major
 * gravitating bodies in the Solar system, provided planet positions are available via a
 * novas_planet_provider function.
 *
 * REFERENCES:
 * <ol>
 * <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 * <li>Explanatory Supplement to the Astronomical Almanac (1992), Chapter 3.</li>
 * </ol>
 *
 * @param jd_tt     [day] Terrestrial Time (TT) based Julian date.
 * @param star      Pointer to catalog entry structure containing catalog data for the object in
 *                  the ICRS.
 * @param accuracy  NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] ra   [h] Virtual apparent right ascension for a fictitous geocentric observer,
 *                  referred to the GCRS (it may be NULL if not required).
 * @param[out] dec  [deg] Virtual apparent declination for a fictitous geocentric observer,
 *                  referred to the GCRS (it may be NULL if not required).
 * @return          0 if successful, or -1 if a required pointer argument is NULL, or 20 + the
 *                  error from place().
 *
 * @sa place_star(), place_gcrs(), app_star(), astro_star(), local_star(), topo_star(),
 *     virtual_planet()
 * @sa novas_sky_pos(), make_observer_at_geocenter(), NOVAS_GCRS
 */
short virtual_star(double jd_tt, const cat_entry *restrict star, enum novas_accuracy accuracy,
        double *restrict ra, double *restrict dec) {
  prop_error("virtual_star", radec_star(jd_tt, star, NULL, 0.0, NOVAS_GCRS, accuracy, ra, dec, NULL), 0);
  return 0;
}

/**
 * __reference_system__: ICRS / GCRS <br>
 * __observer location__: geocenter <br>
 * __position_type__: geometric <br>
 *
 * Computes the astrometric place of a star, as would be seen by a non-moving observer at the
 * current location of the geocenter, referred to the ICRS without light deflection or aberration,
 * at date 'jd_tt', given its catalog mean place, proper motion, parallax, and radial velocity.
 *
 * Notwithstanding the different set of return values, this is the same as calling place_star()
 * with a NULL observer location and NOVAS_ICRS as the system, or place_icrs() for an object that
 * specifies the star.
 *
 * REFERENCES:
 * <ol>
 * <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 * <li>Explanatory Supplement to the Astronomical Almanac (1992), Chapter 3.</li>
 * </ol>
 *
 * @param jd_tt     [day] Terrestrial Time (TT) based Julian date.
 * @param star      Pointer to catalog entry structure containing catalog data for the object in
 *                  the ICRS.
 * @param accuracy  NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] ra   [h] Astrometric (geometric) right ascension for a fictitous geocentric observer,
 *                  referred to the ICRS, without light deflection or aberration. (It may be NULL if
 *                  not required)
 * @param[out] dec  [deg] Astrometric (geometric) declination for a fictitous geocentric observer,
 *                  referred to the ICRS, without light deflection or aberration. (It may be NULL if
 *                  not required)
 * @return          0 if successful, or -1 if a required pointer argument is NULL, or
 *                  20 + the error from place().
 *
 * @sa place_star(), place_icrs(), app_star(), local_star(), topo_star(), virtual_star(),
 *     astro_planet()
 * @sa novas_geom_posvel(), make_observer_at_geocenter(), NOVAS_ICRS
 */
short astro_star(double jd_tt, const cat_entry *restrict star, enum novas_accuracy accuracy,
        double *restrict ra, double *restrict dec) {
  prop_error("astro_star", radec_star(jd_tt, star, NULL, 0.0, NOVAS_ICRS, accuracy, ra, dec, NULL), 0);
  return 0;
}

/**
 * __reference_system__: TOD <br>
 * __observer location__: geocenter <br>
 * __position_type__: apparent <br>
 *
 * Computes the True-of-Date (TOD) apparent place of a solar system body as would be seen by an
 * observer at the geocenter. This is the same as calling place() for the body with NOVAS_TOD as
 * the system and a geocentric observer, except the different set of return values used.
 *
 * The calculated positions and velocities include aberration corrections for the moving frame of
 * the observer as well as gravitational deflection due to the Sun and Earth and other major
 * gravitating bodies in the Solar system, provided planet positions are available via a
 * novas_planet_provider function.
 *
 * REFERENCES:
 * <ol>
 *     <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 *     <li>Explanatory Supplement to the Astronomical Almanac (1992),Chapter 3.</li>
 * </ol>
 *
 * @param jd_tt     [day] Terretrial Time (TT) based Julian date.
 * @param ss_body   Pointer to structure containing the body designation for the solar system body.
 * @param accuracy  NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] ra   [h] Apparent (TOD) right ascension for a fictitous geocentric observer,
 *                  referred to true equator and equinox of date 'jd_tt'. (It may be NULL if not
 *                  required)
 * @param[out] dec  [deg] Apparent (TOD) declination for a fictitous geocentric observer, referred
 *                  to true equator and equinox of date 'jd_tt'. (It may be NULL if not required)
 * @param[out] dis  [AU] Apparent distance from Earth to the body at 'jd_tt' (it may be NULL if
 *                  not needed).
 * @return          0 if successful, or -1 if the object argument is NULL, or else 1 if the value
 *                  of 'type' in structure 'ss_body' is invalid, or 10 + the error from place().
 *
 * @sa place_tod(), astro_planet(), local_planet(), topo_planet(),
 *     virtual_planet(), app_star()
 * @sa novas_sky_pos(), make_observer_at_geocenter(), NOVAS_TOD
 */
short app_planet(double jd_tt, const object *restrict ss_body, enum novas_accuracy accuracy,
        double *restrict ra, double *restrict dec, double *restrict dis) {
  prop_error("app_planet", radec_planet(jd_tt, ss_body, NULL, 0.0, NOVAS_TOD, accuracy, ra, dec, dis, NULL), 0);
  return 0;
}

/**
 * __reference_system__: ICRS / GCRS <br>
 * __observer location__: geocenter <br>
 * __position_type__: apparent <br>
 *
 * Computes the virtual place of a solar system body, as would be seen by an observer at the
 * geocenter, referenced to the GCRS. This is the same as calling place_gcrs() for the body,
 * except the different set of return values used.
 *
 * The calculated positions and velocities include aberration corrections for the moving frame of
 * the observer as well as gravitational deflection due to the Sun and Earth and other major
 * gravitating bodies in the Solar system, provided planet positions are available via a
 * novas_planet_provider function.
 *
 * REFERENCES:
 * <ol>
 *     <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 *     <li>Explanatory Supplement to the Astronomical Almanac (1992),Chapter 3.</li>
 * </ol>
 *
 * @param jd_tt     [day] Terretrial Time (TT) based Julian date.
 * @param ss_body   Pointer to structure containing the body designation for the solar system body.
 * @param accuracy  NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] ra   [h] Virtual apparent right ascension for a fictitous geocentric observer,
 *                  referred to the GCRS (it may be NULL if not required).
 * @param[out] dec  [deg] Virtual apparent declination, for a fictitous geocentric observer,
 *                  referred to the GCRS (it may be NULL if not required).
 * @param[out] dis  [AU] Apparent distance from Earth to the body at 'jd_tt' (it may be NULL if
 *                  not needed).
 * @return          0 if successful, or -1 if the object argument is NULL, or else 1 if the
 *                  value of 'type' in structure 'ss_body' is invalid, or 10 + the error from
 *                  place().
 *
 * @sa place_gcrs(), app_planet(), astro_planet(), local_planet(), topo_planet(), app_star()
 * @sa novas_sky_pos(), make_observer_at_geocenter(), NOVAS_GCRS
 */
short virtual_planet(double jd_tt, const object *restrict ss_body, enum novas_accuracy accuracy,
        double *restrict ra, double *restrict dec, double *restrict dis) {
  prop_error("virtual_planet", radec_planet(jd_tt, ss_body, NULL, 0.0, NOVAS_GCRS, accuracy, ra, dec, dis, NULL), 0);
  return 0;
}

/**
 * __reference_system__: ICRS / GCRS <br>
 * __observer location__: geocenter <br>
 * __position_type__: geometric <br>
 *
 * Computes the astrometric place of a solar system body, as would be seen by a non-moving observer
 * located at the current position of the geocenter, referenced to the ICRS without light
 * deflection or aberration. This is the same as calling place_icrs() for the body, except the
 * different set of return values used.
 *
 * REFERENCES:
 * <ol>
 *     <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 *     <li>Explanatory Supplement to the Astronomical Almanac (1992),Chapter 3.</li>
 * </ol>
 *
 * @param jd_tt     [day] Terretrial Time (TT) based Julian date.
 * @param ss_body   Pointer to structure containing the body designation for the solar system body.
 * @param accuracy  NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] ra   [h] Astrometric (geometric) right ascension for a fictitous geocentric
 *                  observer, referred to the ICRS, without light deflection or aberration. (It
 *                  may be NULL if not required)
 * @param[out] dec  [deg] Astrometric (geometric) declination, for a fictitous geocentric observer,
 *                  referred to the ICRS, without light deflection or aberration. (It may be NULL
 *                  if not required)
 * @param[out] dis  [AU] Apparent distance from Earth to the body at 'jd_tt' (it may be NULL if
 *                  not needed).
 * @return          0 if successful, or -1 if the object is NULL, or else 1 if the value of 'type'
 *                  in structure 'ss_body' is invalid, or 10 + the error from place().
 *
 * @sa place_icrs(), app_planet(), local_planet(), topo_planet(), virtual_planet(), astro_star()
 * @sa novas_geom_posvel(), make_observer_at_geocenter(), NOVAS_GCRS
 */
short astro_planet(double jd_tt, const object *restrict ss_body, enum novas_accuracy accuracy,
        double *restrict ra, double *restrict dec, double *restrict dis) {
  prop_error("astro_planet", radec_planet(jd_tt, ss_body, NULL, 0.0, NOVAS_ICRS, accuracy, ra, dec, dis, NULL), 0);
  return 0;
}

/**
 * @deprecated Using the frame-based novas_sky_pos() with NOVAS_TOD as the reference system is now
 *             preferred for topocentric calculations.
 *
 * __reference_system__: TOD <br>
 * __observer location__: topocentric (on Earth) <br>
 * __position_type__: apparent <br>
 *
 * Computes the topocentric (True of Date; TOD) apparent place of a star at date 'jd_tt', given
 * its ICRS catalog place, proper motion, parallax, and radial velocity.
 *
 * Notwithstanding the different set of return values, this is the same as calling place_star()
 * with the same observer location and NOVAS_TOD for an object that specifies the star.
 *
 * The calculated positions and velocities include aberration corrections for the moving frame of
 * the observer as well as gravitational deflection due to the Sun and Earth and other major
 * gravitating bodies in the Solar system, provided planet positions are available via a
 * novas_planet_provider function.
 *
 * REFERENCES:
 * <ol>
 * <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 * <li>Explanatory Supplement to the Astronomical Almanac (1992), Chapter 3.</li>
 * </ol>
 *
 * @param jd_tt     [day] Terrestrial Time (TT) based Julian date.
 * @param ut1_to_tt [s] Difference TT-UT1 at 'jd_tt', in seconds of time.
 * @param star      Pointer to catalog entry structure containing catalog data for the object in
 *                  the ICRS.
 * @param position  Position of the observer
 * @param accuracy  NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] ra   [h] Topocentric appatent right ascension, referred to true equator and equinox
 *                  of date 'jd_tt'. (It may be NULL if not required)
 * @param[out] dec  [deg] Topocentric apparent declination, referred to true equator and equinox
 *                  of date 'jd_tt'. (It may be NULL if not required)
 * @return          0 if successful, -1 if a required pointer argument is NULL, or else 20 + the
 *                  error from place_star().
 *
 * @sa place_star(), app_star(), local_star(), topo_star(), virtual_star(), astro_planet()
 * @sa novas_sky_pos(), make_observer_at_site(), NOVAS_TOD, get_ut1_to_tt()
 */
short topo_star(double jd_tt, double ut1_to_tt, const cat_entry *restrict star, const on_surface *restrict position,
        enum novas_accuracy accuracy, double *restrict ra, double *restrict dec) {
  static const char *fn = "topo_star";
  observer obs = {0};
  prop_error(fn, make_observer_at_site(position, &obs), 0);
  prop_error(fn, radec_star(jd_tt, star, &obs, ut1_to_tt, NOVAS_TOD, accuracy, ra, dec, NULL), 0);
  return 0;
}

/**
 * @deprecated Using the frame-based novas_sky_pos() with NOVAS_ICRS or NOVARS_GCRS as the
 *             reference system is now preferred for the equivalent calculations.
 *
 * __reference_system__: ICRS / GCRS <br>
 * __observer location__: topocentric (on Earth) <br>
 * __position_type__: apparent <br>
 *
 * Computes the local apparent place of a star at date 'jd_tt', in the GCRS, given its catalog
 * mean place, proper motion, parallax, and radial velocity.
 *
 * Notwithstanding the different set of return values, this is the same as calling place_star()
 * with the same observer location NOVAS_GCRS for an object that specifies the star.
 *
 * The calculated positions and velocities include aberration corrections for the moving frame of
 * the observer as well as gravitational deflection due to the Sun and Earth and other major
 * gravitating bodies in the Solar system, provided planet positions are available via a
 * novas_planet_provider function.
 *
 * REFERENCES:
 * <ol>
 * <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 * <li>Explanatory Supplement to the Astronomical Almanac (1992), Chapter 3.</li>
 * </ol>
 *
 * @param jd_tt     [day] Terrestrial Time (TT) based Julian date.
 * @param ut1_to_tt [s] Difference TT-UT1 at 'jd_tt', in seconds of time.
 * @param star      Pointer to catalog entry structure containing catalog data for the object in
 *                  the ICRS.
 * @param position  Position of the observer
 * @param accuracy  NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] ra   [h] Local apparent right ascension, referred to the GCRS (it may be NULL if
 *                  not required).
 * @param[out] dec  [deg] Local apparent right ascension, referred to the GCRS (it may be NULL if
 *                  not required).
 * @return          0 if successful, or -1 if any of the required pointer arguments is NULL, or
 *                  else 20 + the error from place().
 *
 * @sa place_star(), app_star(), astro_star(), topo_star(), virtual_star(), astro_planet()
 * @sa novas_sky_pos(), make_observer_at_site(), NOVAS_GCRS, get_ut1_to_tt()
 */
short local_star(double jd_tt, double ut1_to_tt, const cat_entry *restrict star, const on_surface *restrict position,
        enum novas_accuracy accuracy, double *restrict ra, double *restrict dec) {
  static const char *fn = "local_star";
  observer obs = {0};
  prop_error(fn, make_observer_at_site(position, &obs), 0);
  prop_error(fn, radec_star(jd_tt, star, &obs, ut1_to_tt, NOVAS_GCRS, accuracy, ra, dec, NULL), 0);
  return 0;
}

/**
 * @deprecated Using the frame-based novas_sky_pos() with NOVAS_TOD as the reference system is now
 *             preferred for topocentric calculations.
 *
 * __reference_system__: TOD <br>
 * __observer location__: topocentric (on Earth) <br>
 * __position_type__: apparent <br>
 *
 * Computes the topocentric apparent place of a solar system body at the specified time. This is
 * the same as calling place() for the body for the same observer location and NOVAS_TOD as the
 * reference system, except the different set of return values used.
 *
 * The calculated positions and velocities include aberration corrections for the moving frame of
 * the observer as well as gravitational deflection due to the Sun and Earth and other major
 * gravitating bodies in the Solar system, provided planet positions are available via a
 * novas_planet_provider function.
 *
 * REFERENCES:
 * <ol>
 *     <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 *     <li>Explanatory Supplement to the Astronomical Almanac (1992),Chapter 3.</li>
 * </ol>
 *
 * @param jd_tt     [day] Terretrial Time (TT) based Julian date.
 * @param ss_body   Pointer to structure containing the body designation for the solar system body.
 * @param ut1_to_tt [s] Difference TT-UT1 at 'jd_tt', in seconds of time.
 * @param position  Position of the observer
 * @param accuracy  NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] ra   [h] Topocentric apparent right ascension, referred to the true equator and
 *                  equinox of date. (It may be NULL if not required)
 * @param[out] dec  [deg] Topocentric apparent declination, referred to the true equator and
 *                  equinox of date. (It may be NULL if not required)
 * @param[out] dis  [AU] Apparent distance from Earth to the body at 'jd_tt' (may be NULL if not
 *                  needed).
 * @return          0 if successful, or -1 if the object argument is NULL, or else 1 if the value
 *                  of 'where' in structure 'location' is invalid, or 10 + the error from place().
 *
 * @sa app_planet(), local_planet(), topo_planet(), virtual_planet(),
 *     astro_star()
 * @sa novas_sky_pos(), make_observer_at_site(), NOVAS_TOD, get_ut1_to_tt()
 */
short topo_planet(double jd_tt, const object *restrict ss_body, double ut1_to_tt, const on_surface *restrict position,
        enum novas_accuracy accuracy, double *restrict ra, double *restrict dec, double *restrict dis) {
  static const char *fn = "topo_planet";
  observer obs = {0};
  prop_error(fn, make_observer_at_site(position, &obs), 0);
  prop_error(fn, radec_planet(jd_tt, ss_body, &obs, ut1_to_tt, NOVAS_TOD, accuracy, ra, dec, dis, NULL), 0);
  return 0;
}

/**
 * @deprecated Using the frame-based novas_sky_pos() with NOVAS_ICRS or NOVARS_GCRS as the
 *             reference system is now preferred for the equivalent calculations.
 *
 * __reference_system__: ICRS/GCRS <br>
 * __observer location__: topocentric (on Earth) <br>
 * __position_type__: apparent <br>
 *
 * Computes the local apparent place of a solar system body, in the GCRS. This is the same as
 * calling place() for the body for the same observer location and NOVAS_GCRS as the reference
 * system, except the different set of return values used.
 *
 * The calculated positions and velocities include aberration corrections for the moving frame of
 * the observer as well as gravitational deflection due to the Sun and Earth and other major
 * gravitating bodies in the Solar system, provided planet positions are available via a
 * novas_planet_provider function.
 *
 * REFERENCES:
 * <ol>
 *     <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 *     <li>Explanatory Supplement to the Astronomical Almanac (1992),Chapter 3.</li>
 * </ol>
 *
 * @param jd_tt     [day] Terrestrial Time (TT) based Julian date.
 * @param ss_body   Pointer to structure containing the body designation for the solar
 *                  system body.
 * @param ut1_to_tt [s] Difference TT-UT1 at 'jd_tt', in seconds of time.
 * @param position  Position of the observer
 * @param accuracy  NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] ra   [h] Local apparent right ascension, referred to the GCRS (it may be NULL if
 *                  not required).
 * @param[out] dec  [deg] Local apparent right ascension, referred to the GCRS (it may be NULL
 *                  if not required).
 * @param[out] dis  [AU] Apparent distance from Earth to the body at 'jd_tt' (it may be NULL if
 *                  not required).
 * @return          0 if successful, or -1 if the object argument is NULL, or else 1 if
 *                  the value of 'where' in structure 'location' is invalid, or 10 + the
 *                  error code from place().
 *
 * @sa astro_planet(), topo_planet(), virtual_planet(), app_star()
 * @sa novas_sky_pos(), make_observer_at_site(), NOVAS_GCRS, get_ut1_to_tt()
 */
short local_planet(double jd_tt, const object *restrict ss_body, double ut1_to_tt, const on_surface *restrict position,
        enum novas_accuracy accuracy, double *restrict ra, double *restrict dec, double *restrict dis) {
  static const char *fn = "local_planet";
  observer obs = {0};
  prop_error(fn, make_observer_at_site(position, &obs), 0);
  prop_error(fn, radec_planet(jd_tt, ss_body, &obs, ut1_to_tt, NOVAS_GCRS, accuracy, ra, dec, dis, NULL), 0);
  return 0;
}

/**
 * __reference system: TOD &rarr; ICRS<br>
 * __observer location__: geocenter &rarr; SSB <br>
 * __position_type__: apparent &rarr; geometric <br>
 *
 * Computes the barycentric ICRS position of a star, given its True of Date (TOD) apparent place, as
 * seen by an observer at the geocenter, at date 'jd_tt'. Proper motion, parallax and radial velocity
 * are assumed to be zero.
 *
 * Equivalently, in the new frame-based approach, you might use `novas_app_to_geom()` with NOVAS_TOD
 * as the reference system, and distance set to 0, and then convert the geometric position to the
 * output `ira`, `idec` coordinates using `vector2radec()`. The advantage of using
 * `novas_app_to_geom()` is that you are not restricted to using TOD input coordinates, but rather
 * the apparent RA/Dec may be specified in any reference system.
 *
 * REFERENCES:
 * <ol>
 *     <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 *     <li>Explanatory Supplement to the Astronomical Almanac (1992),Chapter 3.</li>
 * </ol>
 *
 * @param jd_tt         [day] Terrestrial Time (TT) based Julian date.
 * @param[in] tra       [h] Geocentric apparent (TOD) right ascension, referred to true equator
 *                      and equinox of date.
 * @param[in] tdec      [deg] Geocentric apparent (TOD) declination, referred to true equator and
 *                      equinox of date.
 * @param accuracy      NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] ira      [h] Barycentric geometric ICRS right ascension, or NAN if returning with
 *                      an error code. It may be a pointer to the input value.
 * @param[out] idec     [deg] Barycentric geometric ICRS declination, or NAN if returning with an
 *                      error code. It may be a pointer to the input value.
 * @return              0 if successful; -1 if the supplied output pointers are NULL,
 *                      1 if the iterative process did not converge after 30 iterations, or an
 *                      error from vector2radec(), or else &gt; 10 + an error from app_star().
 *
 * @sa novas_app_to_geom(), NOVAS_TOD, make_observer_at_geocenter(), vector2radec()
 */
short mean_star(double jd_tt, double tra, double tdec, enum novas_accuracy accuracy, double *restrict ira, double *restrict idec) {
  static const char *fn = "mean_star";
  cat_entry star = CAT_ENTRY_INIT;
  double pos[3];
  int iter;

  if(!ira || !idec)
    return novas_error(-1, EINVAL, fn, "NULL output pointer: ira=%p, idec=%p", ira, idec);

  // Default return values...
  *ira = NAN;
  *idec = NAN;

  // create a position vector based on the apparent RA and declination of the star.
  star.ra = tra;
  star.dec = tdec;
  starvectors(&star, pos, NULL);

  // Get initial approximation by precessing star position at 'jd_tt'
  // to its position at J2000.0.
  precession(jd_tt, pos, JD_J2000, pos);

  prop_error(fn, vector2radec(pos, &star.ra, &star.dec), 10);

  // Iteratively find ICRS coordinates that produce input apparent place
  // of star at date 'jd_tt'.
  for(iter = novas_inv_max_iter; --iter >= 0;) {
    double ra1, dec1;

    prop_error(fn, app_star(jd_tt, &star, accuracy, &ra1, &dec1), 20);

    // If within tolerance, we are done
    if(fabs(tra - ra1) < 1.0e-12 && fabs(tdec - dec1) < 1.0e-11) {
      *ira = star.ra < 0.0 ? star.ra + DAY_HOURS : star.ra;
      *idec = star.dec;
      return 0;
    }

    // Correct for overshoot
    star.ra = remainder(star.ra + (tra - ra1), DAY_HOURS);
    star.dec = remainder(star.dec + (tdec - dec1), DEG360);
  }

  return novas_error(1, ECANCELED, fn, "failed to converge");
}

/**
 * __reference_system__: ICRS / GCRS <br>
 * __observer location__: geocenter <br>
 * __position_type__: geometric <br>
 *
 * Computes the International Celestial Reference System (ICRS) position of a source. (from the
 * geocenter). Unlike `place_gcrs()`, this version does not include aberration or gravitational
 * deflection corrections.
 *
 * @param jd_tt       [day] Terrestrial Time (TT) based Julian date of observation.
 * @param source      Catalog source or solar_system body.
 * @param accuracy    NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] pos    Structure to populate with the calculated geometric ICRS position data for a
 *                    fictitous geocentric observer. (Unlike place_gcrs(), the calculated
 *                    coordinates do not account for aberration or gravitational deflection).
 * @return            0 if successful, or -1 if any of the input pointer arguments is NULL, or
 *                    else an error from place().
 *
 * @since 1.0
 * @author Attila Kovacs
 *
 * @sa place_gcrs(), place_cirs(), place_tod(), mean_star()
 * @sa novas_geom_posvel(), NOVAS_GCRS, make_observer_at_geocenter()
 */
int place_icrs(double jd_tt, const object *restrict source, enum novas_accuracy accuracy, sky_pos *restrict pos) {
  prop_error("place_icrs", place(jd_tt, source, NULL, 0.0, NOVAS_ICRS, accuracy, pos), 0);
  return 0;
}

/**
 * __reference_system__: ICRS/GCRS <br>
 * __observer location__: geocenter <br>
 * __position_type__: apparent <br>
 *
 * Computes the Geocentric Celestial Reference System (GCRS) position of a source (as 'seen' from
 * the geocenter) at the given time of observation. Unlike `place_icrs()`, this includes
 * aberration for the moving frame of the geocenter as well as gravitational deflections
 * calculated for a virtual observer located at the geocenter. See `place()` for more information.
 *
 * @param jd_tt       [day] Terrestrial Time (TT) based Julian date of observation.
 * @param source      Catalog source or solar_system body.
 * @param accuracy    NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] pos    Structure to populate with the calculated apparent GCRS position data for a
 *                    fictitous geocentric observer.
 * @return            0 if successful, or -1 if any of the input pointer arguments is NULL, or
 *                    else an error from place().
 *
 * @since 1.0
 * @author Attila Kovacs
 *
 * @sa place_icrs(), place_cirs(), place_tod(), virtual_star(), virtual_planet()
 * @sa novas_sky_pos(), NOVAS_GCRS, make_observer_at_geocenter(),
 */
int place_gcrs(double jd_tt, const object *restrict source, enum novas_accuracy accuracy, sky_pos *restrict pos) {
  prop_error("place_gcrs", place(jd_tt, source, NULL, 0.0, NOVAS_GCRS, accuracy, pos), 0);
  return 0;
}

/**
 * __reference_system__: CIRS <br>
 * __observer location__: geocenter <br>
 * __position_type__: apparent <br>
 *
 * Computes the Celestial Intermediate Reference System (CIRS) dynamical position
 * position of a source as 'seen' from the geocenter at the given time of observation. See
 * `place()` for more information.
 *
 * The calculated positions and velocities include aberration corrections for the moving frame of
 * the observer as well as gravitational deflection due to the Sun and Earth and other major
 * gravitating bodies in the Solar system, provided planet positions are available via a
 * novas_planet_provider function.
 *
 * @param jd_tt       [day] Terrestrial Time (TT) based Julian date of observation.
 * @param source      Catalog source or solar_system body.
 * @param accuracy    NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] pos    Structure to populate with the calculated apparent CIRS position data for a
 *                    fictitous geocentric observer.
 * @return            0 if successful, or -1 if any of the input pointer arguments is NULL, or
 *                    else an error from place().
 *
 * @since 1.0
 * @author Attila Kovacs
 *
 * @sa place_tod(), place_gcrs()
 * @sa novas_sky_pos(), NOVAS_CIRS, make_observer_at_geocenter()
 */
int place_cirs(double jd_tt, const object *restrict source, enum novas_accuracy accuracy, sky_pos *restrict pos) {
  prop_error("place_cirs", place(jd_tt, source, NULL, 0.0, NOVAS_CIRS, accuracy, pos), 0);
  return 0;
}

/**
 * __reference_system__: TOD <br>
 * __observer location__: geocenter <br>
 * __position_type__: apparent <br>
 *
 * Computes the True of Date (TOD) dynamical position position of a source as 'seen' from the
 * geocenter at the given time of observation. See `place()` for more information.
 *
 * The calculated positions and velocities include aberration corrections for the moving frame of
 * the observer as well as gravitational deflection due to the Sun and Earth and other major
 * gravitating bodies in the Solar system, provided planet positions are available via a
 * novas_planet_provider function.
 *
 * @param jd_tt       [day] Terrestrial Time (TT) based Julian date of observation.
 * @param source      Catalog source or solar_system body.
 * @param accuracy    NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] pos    Structure to populate with the calculated apparent True-of-Date (TOD)
 *                    position data for a fictitous geocentric observer.
 * @return            0 if successful, or -1 if any of the input pointer arguments is NULL, or
 *                    else an error from place().
 *
 * @since 1.0
 * @author Attila Kovacs
 *
 * @sa place_cirs(), place_gcrs(), app_star(), app_planet()
 * @sa novas_sky_pos(), NOVAS_TOD, make_observer_at_geocenter()
 */
int place_tod(double jd_tt, const object *restrict source, enum novas_accuracy accuracy, sky_pos *restrict pos) {
  prop_error("place_tod", place(jd_tt, source, NULL, 0.0, NOVAS_TOD, accuracy, pos), 0);
  return 0;
}

/**
 * __reference_system__: MOD <br>
 * __observer location__: geocenter <br>
 * __position_type__: apparent <br>
 *
 * Computes the Mean of Date (MOD) dynamical position position of a source as 'seen' from the
 * geocenter at the given time of observation. See `place()` for more information.
 *
 * The calculated positions and velocities include aberration corrections for the moving frame of
 * the observer as well as gravitational deflection due to the Sun and Earth and other major
 * gravitating bodies in the Solar system, provided planet positions are available via a
 * novas_planet_provider function.
 *
 * @param jd_tt       [day] Terrestrial Time (TT) based Julian date of observation.
 * @param source      Catalog source or solar_system body.
 * @param accuracy    NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] pos    Structure to populate with the calculated apparent Mean-of-Date (MOD)
 *                    position data for a fictitous geocentric observer.
 * @return            0 if successful, or -1 if any of the input pointer arguments is NULL, or
 *                    else an error from place().
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa place_cirs(), place_gcrs(), app_star(), app_planet()
 * @sa novas_sky_pos(), NOVAS_TOD, make_observer_at_geocenter()
 */
int place_mod(double jd_tt, const object *restrict source, enum novas_accuracy accuracy, sky_pos *restrict pos) {
  prop_error("place_mod", place(jd_tt, source, NULL, 0.0, NOVAS_MOD, accuracy, pos), 0);
  return 0;
}

/**
 * __reference_system__: J2000 <br>
 * __observer location__: geocenter <br>
 * __position_type__: apparent <br>
 *
 * Computes the J2000 dynamical position position of a source as 'seen' from the geocenter at the
 * given time of observation. See `place()` for more information.
 *
 * The calculated positions and velocities include aberration corrections for the moving frame of
 * the observer as well as gravitational deflection due to the Sun and Earth and other major
 * gravitating bodies in the Solar system, provided planet positions are available via a
 * novas_planet_provider function.
 *
 * @param jd_tt       [day] Terrestrial Time (TT) based Julian date of observation.
 * @param source      Catalog source or solar_system body.
 * @param accuracy    NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] pos    Structure to populate with the calculated apparent J2000 position data for
 *                    a fictitous geocentric observer.
 * @return            0 if successful, or -1 if any of the input pointer arguments is NULL, or
 *                    else an error from place().
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa place_cirs(), place_gcrs(), app_star(), app_planet()
 * @sa novas_sky_pos(), NOVAS_TOD, make_observer_at_geocenter()
 */
int place_j2000(double jd_tt, const object *restrict source, enum novas_accuracy accuracy, sky_pos *restrict pos) {
  prop_error("place_j2000", place(jd_tt, source, NULL, 0.0, NOVAS_J2000, accuracy, pos), 0);
  return 0;
}

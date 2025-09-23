/**
 * @file
 *
 * @date Created  on Mar 6, 2025
 * @author G. Kaplan and Attila Kovacs
 *
 *  This module contains functions that define an observing target, or which are target related.
 *  In general, the @ref object data structure contains information on the astronomical target of
 *  observation. Depending on the type of astronomical source, you might use different functions
 *  to define the observing target:
 *
 *  - For __major planets__, Sun, Moon, Solar-system barycenter (SSB), Earth-Moon barycenter
 *    (EMB), or the Pluto system barycenter: Use `make_planet()` or else one of the static
 *    initializers, such as `NOVAS_MARS_INIT`.
 *
 *  - For __Solar-system bodies__, that is everything with ephemeris data other than a 'major
 *    planet', such as asteroids, comets, moons, spacecraft, you'd use `make_ephem_object()`.
 *
 *  - For __Keplerian orbital elements__, such as the ones published by the Minor Planet Center
 *    (MPC) for asteroids, comets, or Near-Earth Objects (NEOs), you will want to use
 *    `make_orbital_object()`.
 *
 *  - For __catalog sources__, especially within our own Galaxy, you might start with
 *    `novas_init_cat_entry()` and then set additional parameters as needed, and then define an
 *    @ref object with ICRS coordinates, e.g. via `make_cat_object_sys()`.
 *
 *  - For __extragalactic sources__ (with no proper motion or parallax) you might use
 *    `novas_make_redshifted_object()` with ICRS coordinates, or else
 *    `novas_make_redshifted_object_sys()` with coordinates in other reference systems.
 *
 * By default SuperNOVAS treats object names as case insensitive (for historical reasons), and
 * will use capitalized versions of the names supplied by the user. You can enable case-sensitive
 * name processing with `novas_case_sensitive()` if needed.
 *
 * Once an @ref object is defined, you may calculate astrometric properties, such as apparent or
 * geometric positions; rise, set, or transit times, etc. through observing frames, which are
 * defined for a specific observer location and time of observation.
 *
 * @sa frame.c, observer.c, timescale.c
 */

/// \cond PRIVATE
#define _GNU_SOURCE               ///< for strcasecmp()
#define __NOVAS_INTERNAL_API__    ///< Use definitions meant for internal use by SuperNOVAS only
/// \endcond

#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>   // stdarg.h before stdio.h (for older gcc...)
#include <stddef.h>

#include "novas.h"

#if __Lynx__ && __powerpc__
// strcasecmp() / strncasecmp() are not defined on PowerPC / LynxOS 3.1
int strcasecmp(const char *s1, const char *s2);
#endif

static int is_case_sensitive = 0; ///< (boolean) whether object names are case-sensitive.

/**
 * Initializes a catalog entry, such as a star or a distant galaxy, with a name and coordinates.
 * All other fields of the `cat_entry` structure will be initialized with zeroes. After the
 * initialization, you may set further properties, such as:
 *
 * - a radial velocity, LSR velocity, or redshift.
 * - a parallax or distance
 * - proper motion
 * - optional catalog name an number
 *
 * The typical process for setting up a catalog source is captured by the following steps:
 *
 * 1. Initialize the `cat_entry` with source name and RA / Dec coordinates using
 *    `novas_init_cat_entry()`.
 * 2. (optional) Set radial velocity, as needed, via either:
 *     - radial velocity (km/s) w.r.t SSB using `novas_set_ssb_vel()`, or
 *     - LSR velocity (km/s) using `novas_set_lsr_vel()`, or
 *     - redshift using `novas_set_redshift()`.
 * 3. (optional) Set proper motions (mas/yr), as needed, using `novas_set_proper_motion()`.
 * 4. (optional) Set parallax, as needed, via either:
 *     - parallax (mas) using `novas_set_parallax()`, or
 *     - distance (parcsec) using `novas_set_distance()`.
 * 5. (optional) Assign catalog info if desired via `novas_set_catalog()`.
 *
 * After the initialization (Step 1), order does not matter.
 *
 * @param[out] source   Output structure to populate.
 * @param name          Object name (less than SIZE_OF_OBJ_NAME in length). It may be NULL if not
 *                      relevant.
 * @param ra            [h] Right ascension of the object.
 * @param dec           [deg] Declination of the object.
 * @return              0 if successful, or else -1 in the source is NULL (errno set to EINVAL) or
 *                      1 if the name is too long (errno is set to ERANGE).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_set_ssb_vel(), novas_set_lsr_vel(), novas_set_redshift(), novas_set_proper_motion(),
 *     novas_set_parallax(), novas_set_distance(), novas_set_cat_info(), novas_make_cat_entry()
 * @sa novas_str_hours(), novas_str_degrees()
 */
int novas_init_cat_entry(cat_entry *restrict source, const char *restrict name, double ra, double dec) {
  static const char *fn = "novas_init_cat_entry";

  if(!source)
    return novas_error(-1, EINVAL, fn, "NULL input 'source'");

  memset(source, 0, sizeof(cat_entry));

  source->ra = ra;
  source->dec = dec;

  if(!name)
    return 0;

  strncpy(source->starname, name, SIZE_OF_OBJ_NAME - 1);
  if(strlen(name) >= SIZE_OF_OBJ_NAME)
     return novas_error(1, ERANGE, fn, "input 'name' is too long (%d > %d)", (int) strlen(name), SIZE_OF_OBJ_NAME - 1);

  return 0;
}

/**
 * Sets the optional catalog information for a sidereal source. SuperNOVAS does not use the
 * catalog information internally. It is entirely for the convenience of the user to set these
 * values if it is useful to them.
 *
 * @param[out] source   Output structure to populate with the parameters.
 * @param catalog       Catalog identifier or epoch (less than SIZE_OF_CAT_NAME in length). E.g.
 *                      'HIP' for Hipparcos, 'TY2' for Tycho-2; or 'ICRS', 'B1950', 'J2000'. It may
 *                      be NULL if not relevant.
 * @param num           Object's number in catalog.
 * @return              0 if successful, or else -1 in the source is NULL (errno set to EINVAL),
 *                      or 2 if the catalog name is too long (errno is set to ERANGE).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_init_cat_entry()
 */
int novas_set_catalog(cat_entry *restrict source, const char *restrict catalog, long num) {
  static const char *fn =  "novas_set_cat_info";

  if(!source)
    return novas_error(-1, EINVAL, fn, "NULL input 'source'");

  source->starnumber = num;

  if(!catalog)
    return 0;

  strncpy(source->catalog, catalog, SIZE_OF_CAT_NAME - 1);
  if(strlen(catalog) >= SIZE_OF_CAT_NAME)
    return novas_error(2, ERANGE, fn, "Input catalog ID is too long (%d > %d)", (int) strlen(catalog), SIZE_OF_CAT_NAME - 1);

  return 0;
}

/**
 * Sets a radial velocity for a catalog source, defined w.r.t. the Solar-System Barycenter (SSB).
 *
 * @param[out] source Output structure to populate with the parameters.
 * @param v_kms       [km/s] Radial velocity of source w.r.t. the Solar-System Barycenter (SSB).
 * @return            0 if successful, or else -1 in the source is NULL (errno is set to EINVAL)
 *                    or if the velocity exceeds the speed of light (errno set to ERANGE).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_set_lsr_vel()
 * @sa novas_set_redshift()
 * @sa novas_init_cat_entry()
 */
int novas_set_ssb_vel(cat_entry *source, double v_kms) {
  const char *fn = "novas_set_ssb_vel";

  if(!source)
    return novas_error(-1, EINVAL, fn, "NULL input 'source'");

  if(1000.0 * fabs(v_kms) > NOVAS_C)
    return novas_error(-1, ERANGE, fn, "input velocity exceeds the speed of light");

  source->radialvelocity = v_kms;
  return 0;
}

/**
 * Sets a radial velocity for a catalog source, defined w.r.t. the Local Standard of Rest (LSR).
 *
 * @param[out] source  Output structure to populate with the parameters.
 * @param v_kms        [km/s] Radial velocity of source w.r.t. the Local Standard of Rest (LSR).
 * @param epoch        [yr] Coordinate epoch.
 * @return             0 if successful, or else -1 in the source is NULL (errno is set to EINVAL)
 *                     or if the velocity exceeds the speed of light (errno set to ERANGE).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_set_ssb_vel()
 * @sa novas_lsr_to_ssb_vel()
 * @sa novas_set_redshift()
 * @sa novas_init_cat_entry()
 */
int novas_set_lsr_vel(cat_entry *source, double epoch, double v_kms) {
  const char *fn = "novas_set_lsr_vel";

  if(!source)
    return novas_error(-1, EINVAL, fn, "NULL input 'source'");

  prop_error(fn, novas_set_ssb_vel(source, novas_lsr_to_ssb_vel(epoch, source->ra, source->dec, v_kms)), 0);
  return 0;
}

/**
 * Sets a redhift for a catalog source, as a relativistic measure of velocity.
 *
 * @param[out] source  Output structure to populate with the parameters.
 * @param z            [-1:inf] The redshift measure _z_, defined as (1 + _z_) =
 *                     sqrt((1 + _v_/_c_) / (1 - _v_/_c_))
 * @return             0 if successful, or else -1 in the source is NULL (errno is set to EINVAL)
 *                     or if the redshift is invalid (errno set to ERANGE).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_set_ssb_vel()
 * @sa novas_init_cat_entry()
 */
int novas_set_redshift(cat_entry *source, double z) {
  double v = novas_z2v(z);

  if(isnan(v))
    return novas_error(-1, ERANGE, "novas_set_redshift", "invalid redshift value: %f\n", z);

  prop_error("novas_set_lsr_vel", novas_set_ssb_vel(source, v), 0);
  return 0;
}

/**
 * Sets the proper-motion for a (Galactic) catalog source.
 *
 * @param[out] source  Output structure to populate with the parameters.
 * @param pm_ra        [mas/yr] Proper motion in right ascension.
 * @param pm_dec       [mas/yr] Proper motion in declination.
 * @return             0 if successful, or else -1 in the source is NULL (errno is set to EINVAL).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_set_parallax()
 * @sa novas_set_distance()
 * @sa novas_init_cat_entry()
 */
int novas_set_proper_motion(cat_entry *source, double pm_ra, double pm_dec) {
  if(!source)
    return novas_error(-1, EINVAL, "novas_set_proper_motion", "NULL input 'source'");

  source->promora = pm_ra;
  source->promodec = pm_dec;
  return 0;
}

/**
 * Sets the parallax (a measure of distance) for a catalog source.
 *
 * @param[out] source  Output structure to populate with the parameters.
 * @param mas          [mas] Parallax
 * @return             0 if successful, or else -1 in the source is NULL (errno is set to EINVAL).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_set_distance()
 * @sa novas_init_cat_entry()
 */
int novas_set_parallax(cat_entry *source, double mas) {
  if(!source)
    return novas_error(-1, EINVAL, "novas_set_parallax", "NULL input 'source'");

  source->parallax = fabs(mas);
  return 0;
}

/**
 * Sets the distance for a catalog source.
 *
 * @param[out] source  Output structure to populate with the parameters.
 * @param parsecs      [pc] distance
 * @return             0 if successful, or else -1 in the source is NULL (errno is set to EINVAL).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_set_distance()
 * @sa novas_init_cat_entry()
 */
int novas_set_distance(cat_entry *source, double parsecs) {
  prop_error("novas_set_lsr_vel", novas_set_parallax(source, 1000.0 / parsecs), 0);
  return 0;
}

/**
 * Fully populates the data structure for a catalog source, such as a star, with all parameters
 * provided at once.
 *
 * Alternatively, you may use `novas_init_cat_entry()` to initialize just with the name and
 * R.A./Dec coordinates, and then add further information step-by-step as needed, icluding using
 * alternative parameters (SSB vs. LSR velocity, vs. redshift; parallax vs. distance). The latter
 * approach provides more flexibility, and will result in more readable code, which is also easier
 * to debug.
 *
 * @param name        Object name (less than SIZE_OF_OBJ_NAME in length). It may be NULL if not
 *                    relevant.
 * @param catalog     Catalog identifier or epoch (less than SIZE_OF_CAT_NAME in length). E.g.
 *                    'HIP' for Hipparcos, 'TY2' for Tycho-2; or 'ICRS', 'B1950', 'J2000'. It may
 *                    be NULL if not relevant.
 * @param cat_num     Object number in the catalog. It is not used internally, so you may set it
 *                    to anything that is meaningful to you, or set it to 0 as a default.
 * @param ra          [h] Right ascension of the object.
 * @param dec         [deg] Declination of the object.
 * @param pm_ra       [mas/yr] Proper motion in right ascension.
 * @param pm_dec      [mas/yr] Proper motion in declination.
 * @param parallax    [mas] Parallax.
 * @param rad_vel     [km/s] Radial velocity relative to the Solar-System Barycenter (SSB). To
 *                    convert velocities defined against the Local Standard of Rest (LSR), you may
 *                    use `novas_lsr_to_ssb_vel()` to convert appropriately. Or, to convert from
 *                    a redshift value, you might use `novas_z2v()`.
 * @param[out] source Pointer to data structure to populate.
 * @return            0 if successful, or -1 if the output argument is NULL, 1 if the 'star_name'
 *                    is too long or 2 if the 'catalog' name is too long.
 *
 * @sa novas_init_cat_entry()
 * @sa novas_lsr_to_ssb_vel()
 * @sa make_redshifted_cat_entry()
 * @sa make_cat_object_sys()
 * @sa transform_cat()
 */
short make_cat_entry(const char *restrict name, const char *restrict catalog, long cat_num, double ra, double dec, double pm_ra, double pm_dec,
        double parallax, double rad_vel, cat_entry *source) {
  static const char *fn = "make_cat_entry";

  prop_error(fn, novas_init_cat_entry(source, name, ra, dec), 0);
  prop_error(fn, novas_set_catalog(source, catalog, cat_num), 0);
  novas_set_proper_motion(source, pm_ra, pm_dec);
  novas_set_parallax(source, parallax);
  prop_error(fn, novas_set_ssb_vel(source, rad_vel), 0);

  return 0;
}

/**
 * Enables or disables case-sensitive processing of the object name. The effect is not
 * retroactive. The setting will only affect the celestial objects that are defined after
 * the call. Note, that catalog names, set via make_cat_entry() are always case sensitive
 * regardless of this setting.
 *
 * @param value   (boolean) TRUE (non-zero) to enable case-sensitive object names, or else
 *                FALSE (0) to convert names to upper case only (NOVAS C compatible
 *                behavior).
 *
 * @sa make_object()
 *
 * @since 1.0
 * @author Attila Kovacs
 */
void novas_case_sensitive(int value) {
  is_case_sensitive = (value != 0);
}

/**
 * @deprecated More functionality and more readable, code results from using one of the specific
 *             alternatives: `make_cat_object()`, `make_redshifted_object()`, `make_planet()`,
 *             `make_ephem_object()`, or `make_orbital_object()`.
 *
 * Populates an object data structure using the parameters provided. By default (for
 * compatibility with NOVAS C) source names are converted to upper-case internally. You can
 * however enable case-sensitive processing by calling novas_case_sensitive() before.
 *
 * NOTES:
 * <ol>
 * <li>This call does not initialize the `orbit` field (added in v1.2) with zeroes to remain ABI
 * compatible with versions &lt;1.2.</li>
 * </ol>
 *
 * @param type          The type of object. NOVAS_PLANET (0), NOVAS_EPHEM_OBJECT (1) or
 *                      NOVAS_CATALOG_OBJECT (2), or NOVAS_ORBITAL_OBJECT (3).
 * @param number        The novas ID number (for solar-system bodies only, otherwise ignored)
 * @param name          The name of the object (case insensitive). It should be shorter than
 *                      SIZE_OF_OBJ_NAME or else an error will be returned. The name is
 *                      converted to upper internally unless novas_case_sensitive() was called
 *                      before to change that.
 * @param star          Pointer to structure to populate with the catalog data for a celestial
 *                      object located outside the solar system. Used only if type is
 *                      NOVAS_CATALOG_OBJECT, otherwise ignored and can be NULL.
 * @param[out] source   Pointer to the celestial object data structure to be populated.
 * @return              0 if successful, or -1 if 'cel_obj' is NULL or when type is
 *                      NOVAS_CATALOG_OBJECT and 'star' is NULL, or else 1 if 'type' is
 *                      invalid, 2 if 'number' is out of legal range or 5 if 'name' is too long.
 *
 * @sa novas_case_sensitive()
 * @sa make_cat_object()
 * @sa make_redshifted_object()
 * @sa make_planet()
 * @sa make_ephem_object()
 * @sa make_orbital_object()
 * @sa novas_make_frame()
 */
short make_object(enum novas_object_type type, long number, const char *name, const cat_entry *star, object *source) {
  static const char *fn = "make_object";

  if(!source)
    return novas_error(-1, EINVAL, fn, "NULL input source");

  // FIXME [v2] will not need special case in v2.x
  memset(source, 0, offsetof(object, orbit));

  // Set the object type.
  if(type < 0 || type >= NOVAS_OBJECT_TYPES)
    return novas_error(1, EINVAL, fn, "invalid input 'type': %d", type);

  source->type = type;

  // Set the object number.
  if(type == NOVAS_PLANET)
    if(number < 0 || number >= NOVAS_PLANETS)
      return novas_error(2, EINVAL, fn, "planet number %ld is out of bounds [0:%d]", number, NOVAS_PLANETS - 1);

  source->number = number;

  if(name) {
    int i;

    for(i = 0; name[i]; i++) {
      // if input name is not null-terminated return error;
      if(i == (sizeof(source->name) - 1))
        return novas_error(5, EINVAL, fn, "unterminated source name");

      source->name[i] = is_case_sensitive ? name[i] : toupper(name[i]);
    }
  }

  // Populate the astrometric-data structure if the object is 'type' = 2.
  if(type == NOVAS_CATALOG_OBJECT) {
    if(!star)
      return novas_error(-1, EINVAL, fn, "NULL input 'star'");

    source->star = *star;
  }

  return 0;
}

/**
 * Sets a celestial object to be a major planet, or the Sun, Moon, Solar-system Barycenter, etc.
 *
 * @param num           Planet ID number (NOVAS convention)
 * @param[out] planet   Pointer to structure to populate.
 * @return              0 if successful, or else -1 if the 'planet' pointer is NULL.
 *
 * @sa make_ephem_object()
 * @sa make_orbital_object()
 * @sa make_cat_object()
 * @sa make_redshifted_object()
 * @sa novas_case_sensitive()
 * @sa novas_make_frame()
 *
 * @since 1.0
 * @author Attila Kovacs
 */
int make_planet(enum novas_planet num, object *restrict planet) {
  static const char *fn = "make_planet";
  const char *names[] = NOVAS_PLANET_NAMES_INIT;

  if(num < 0 || num >= NOVAS_PLANETS)
    return novas_error(-1, EINVAL, fn, "planet number %d is out of bounds [%d:%d]", num, 0, NOVAS_PLANETS - 1);

  prop_error(fn, (make_object(NOVAS_PLANET, num, names[num], NULL, planet) ? -1 : 0), 0);
  return 0;
}

/**
 * Populates and object data structure with the data for a catalog source. The astrometric
 * parameters must be defined with ICRS. To create objects in other reference systems, use
 * `make_cat_object_sys()` instead.
 *
 * @param star          Pointer to structure to populate with the catalog data for a celestial
 *                      object located outside the solar system, specified with ICRS astrometric
 *                      parameters.
 * @param[out] source   Pointer to the celestial object data structure to be populated.
 * @return              0 if successful, or -1 if either argument is NULL, or else 5 if 'name' is
 *                      too long.
 *
 * @sa make_cat_object_sys()
 * @sa novas_init_cat_entry()
 * @sa make_redshifted_object()
 * @sa make_planet()
 * @sa make_ephem_object()
 * @sa make_orbital_object()
 * @sa novas_case_sensitive()
 * @sa novas_make_frame()
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 */
int make_cat_object(const cat_entry *star, object *source) {
  if(!star || !source)
    return novas_error(-1, EINVAL, "make_cat_object", "NULL parameter: star=%p, source=%p", star, source);
  make_object(NOVAS_CATALOG_OBJECT, star->starnumber, star->starname, star, source);
  return 0;
}

static int cat_to_icrs(cat_entry *restrict star, const char *restrict system) {
  if(strcasecmp(system, "ICRS") != 0) {
    double jd = novas_epoch(system);
    if(isnan(jd))
      return novas_trace("cat_to_icrs", -1, 0);

    if(jd != NOVAS_JD_J2000)
      transform_cat(CHANGE_EPOCH, jd, star, NOVAS_JD_J2000, NOVAS_SYSTEM_FK5, star);

    // Then convert J2000 coordinates to ICRS (also in place). Here the dates don't matter...
    transform_cat(CHANGE_J2000_TO_ICRS, 0.0, star, 0.0, NOVAS_SYSTEM_ICRS, star);
  }
  return 0;
}

/**
 * Populates and object data structure with the data for a catalog source for a given system of
 * catalog coordinates.
 *
 * @param star          Pointer to structure to populate with the catalog data for a celestial
 *                      object located outside the solar system.
 * @param system         Input catalog coordinate system epoch, e.g. "ICRS", "B1950.0", "J2000.0",
 *                      "FK4", "FK5", or "HIP". In general, any Besselian or Julian year epoch can
 *                      be used by year (e.g. "B1933.193" or "J2022.033"), or else the fixed value
 *                      listed. If 'B' or 'J' is ommitted in front of the epoch year, then Besselian
 *                      epochs are assumed prior to 1984.0. (See `novas_epoch() for more).
 * @param[out] source   Pointer to the celestial object data structure to be populated with
 *                      the corresponding ICRS catalog coordinates, after appying proper-motion
 *                      and precession corrections as appropriate.
 * @return              0 if successful, or -1 if any argument is NULL or if the input 'system' is
 *                      invalid, or else 5 if 'name' is too long.
 *
 * @sa make_cat_object()
 * @sa make_redshifted_object_sys()
 * @sa novas_case_sensitive()
 * @sa novas_epoch()
 * @sa novas_make_frame()
 * @sa NOVAS_SYSTEM_ICRS
 * @sa NOVAS_SYSTEM_HIP
 * @sa NOVAS_SYSTEM_J2000
 * @sa NOVAS_SYSTEM_B1950
 *
 * @since 1.3
 * @author Attila Kovacs
 */
int make_cat_object_sys(const cat_entry *star, const char *restrict system, object *source) {
  static const char *fn = "make_cat_object_epoch";

  if(!system)
    return novas_error(-1, EINVAL, fn, "coordinate system is NULL");

  prop_error(fn, make_cat_object(star, source), 0);
  prop_error(fn, cat_to_icrs(&source->star, system), 0);

  return 0;
}

/**
 * Populates a celestial object data structure with the parameters for a redhifted catalog
 * source, such as a distant quasar or galaxy. It is similar to `make_cat_object()` except
 * that it takes a Doppler-shift (z) instead of radial velocity and it assumes no parallax
 * and no proper motion (appropriately for a distant redshifted source). The catalog name
 * is set to `EXT` to indicate an extragalactic source, and the catalog number defaults to 0.
 * The user may change these default field values as appropriate afterwards, if necessary.
 *
 * @param name        Object name (less than SIZE_OF_OBJ_NAME in length). It may be NULL.
 * @param ra          [h] Right ascension of the object (hours).
 * @param dec         [deg] Declination of the object (degrees).
 * @param z           Redhift value (&lambda;<sub>obs</sub> / &lambda;<sub>rest</sub> - 1 =
 *                    f<sub>rest</sub> / f<sub>obs</sub> - 1).
 * @param[out] source Pointer to structure to populate.
 * @return            0 if successful, or 5 if 'name' is too long, else -1 if the 'source'
 *                    pointer is NULL.
 *
 * @sa make_redshifted_object_sys()
 * @sa novas_set_cat_info()
 * @sa novas_v2z()
 *
 * @since 1.2
 * @author Attila Kovacs
 */
int make_redshifted_cat_entry(const char *name, double ra, double dec, double z, cat_entry *source) {
  static const char *fn = "make_redshifted_cat_entry";

  prop_error(fn, novas_init_cat_entry(source, name, ra, dec), 0);
  prop_error(fn, novas_set_redshift(source, z), 0);
  novas_set_catalog(source, "EXT", 0);

  return 0;
}

/**
 * Populates a celestial object data structure with the ICRS astrometric parameters for a
 * redhifted catalog source, such as a distant quasar or galaxy. To create redshifted objects with
 * in other reference systems, use `make_redshifted_object_sys()` instead.
 *
 * It is similar to `make_cat_object()` except that it takes a Doppler-shift (z) instead of radial
 * velocity and it assumes no parallax and no proper motion (appropriately for a distant
 * redshifted source). The catalog name is set to `EXT` to indicate an extragalactic source, and
 * the catalog number defaults to 0. The user may change these default field values as appropriate
 * afterwards, if necessary.
 *
 * @param name        Object name (less than SIZE_OF_OBJ_NAME in length). It may be NULL.
 * @param ra          [h] ICRS Right ascension of the object (hours).
 * @param dec         [deg] ICRS Declination of the object (degrees).
 * @param z           Redhift value (&lambda;<sub>obs</sub> / &lambda;<sub>rest</sub> - 1 =
 *                    f<sub>rest</sub> / f<sub>obs</sub> - 1).
 * @param[out] source Pointer to structure to populate.
 * @return            0 if successful, or 5 if 'name' is too long, else -1 if the 'source'
 *                    pointer is NULL.
 *
 * @sa make_redshifted_object_sys(), make_cat_object(), novas_v2z()
 * @sa novas_str_hours(), novas_str_degrees()
 *
 * @since 1.2
 * @author Attila Kovacs
 */
int make_redshifted_object(const char *name, double ra, double dec, double z, object *source) {
  static const char *fn = "make_redshifted_source";

  cat_entry c;

  prop_error(fn, make_redshifted_cat_entry(name, ra, dec, z, &c), 0);
  prop_error(fn, make_cat_object(&c, source), 0);
  return 0;
}

/**
 * Populates a celestial object data structure with the parameters for a redhifted catalog
 * source, such as a distant quasar or galaxy, for a given system of catalog coordinates.
 *
 * @param name          Object name (less than SIZE_OF_OBJ_NAME in length). It may be NULL.
 * @param ra            [h] ICRS Right ascension of the object (hours).
 * @param dec           [deg] ICRS Declination of the object (degrees).
 * @param system        Input catalog coordinate system epoch, e.g. "ICRS", "B1950.0", "J2000.0",
 *                      "FK4", "FK5", or "HIP". In general, any Besselian or Julian year epoch
 *                      can be used by year (e.g. "B1933.193" or "J2022.033"), or else the fixed
 *                      value listed. If 'B' or 'J' is ommitted in front of the epoch year, then
 *                      Besselian epochs are assumed prior to 1984.0.
 * @param z             Redhift value (&lambda;<sub>obs</sub> / &lambda;<sub>rest</sub> - 1 =
 *                      f<sub>rest</sub> / f<sub>obs</sub> - 1).
 * @param[out] source   Pointer to the celestial object data structure to be populated with
 *                      the corresponding ICRS catalog coordinates.
 * @return              0 if successful, or -1 if any of the pointer arguments is NULL or the
 *                      'system' is invalid, or else 1 if 'type' is invalid, 2 if 'number' is
 *                      out of legal range or 5 if 'name' is too long.
 *
 *
 * @sa make_redshifted_object(), make_cat_object_sys(), novas_epoch(), novas_case_sensitive(),
 *     novas_make_frame()
 * @sa novas_str_hours(), novas_str_degrees(), NOVAS_SYSTEM_ICRS, NOVAS_SYSTEM_HIP,
 *     NOVAS_SYSTEM_J2000, NOVAS_SYSTEM_B1950
 *
 * @since 1.3
 * @author Attila Kovacs
 */
int make_redshifted_object_sys(const char *name, double ra, double dec, const char *restrict system, double z, object *source) {
  static const char *fn = "make_redshifted_object_epoch";

  if(!system)
    return novas_error(-1, EINVAL, fn, "coordinate system is NULL");

  prop_error(fn, make_redshifted_object(name, ra, dec, z, source), 0);
  prop_error(fn, cat_to_icrs(&source->star, system), 0);

  return 0;
}

/**
 * Sets a celestial object to be a Solar-system ephemeris body. Typically this would be used to
 * define minor planets, asteroids, comets and planetary satellites.
 *
 * @param name          Name of object. By default converted to upper-case, unless
 *                      novas_case_sensitive() was called with a non-zero argument. Max.
 *                      SIZE_OF_OBJ_NAME long, including termination. If the ephemeris provider
 *                      uses names, then the name should match those of the ephemeris provider --
 *                      otherwise it is not important.
 * @param num           Solar-system body ID number (e.g. NAIF). The number should match the needs
 *                      of the ephemeris provider used with NOVAS. (If the ephemeris provider is
 *                      by name and not ID number, then the number here is not important).
 * @param[out] body     Pointer to structure to populate.
 * @return              0 if successful, or else -1 if the 'body' pointer is NULL or the name
 *                      is too long.
 *
 *
 * @sa set_ephem_provider()
 * @sa make_planet()
 * @sa make_orbital_object()
 * @sa make_cat_object()
 * @sa make_redshifted_object()
 * @sa novas_case_sensitive()
 * @sa novas_make_frame()
 *
 * @since 1.0
 * @author Attila Kovacs
 */
int make_ephem_object(const char *name, long num, object *body) {
  prop_error("make_ephem_object", (make_object(NOVAS_EPHEM_OBJECT, num, name, NULL, body) ? -1 : 0), 0);
  return 0;
}

/**
 * Sets a celestial object to be a Solar-system orbital body. Typically this would be used to
 * define minor planets, asteroids, comets, or even planetary satellites.
 *
 * @param name          Name of object. It may be NULL if not relevant.
 * @param num           Solar-system body ID number (e.g. NAIF). It is not required and can be set
 *                      e.g. to -1 if not relevant to the caller.
 * @param orbit         The orbital parameters to adopt. The data will be copied, not referenced.
 * @param[out] body     Pointer to structure to populate.
 * @return              0 if successful, or else -1 if the 'orbit' or 'body' pointer is NULL or
 *                      the name is too long.
 *
 *
 * @sa novas_orbit_posvel()
 * @sa make_planet()
 * @sa make_ephem_object()
 * @sa make_cat_object()
 * @sa make_redshifted_object()
 * @sa novas_case_sensitive()
 * @sa novas_make_frame()
 *
 * @since 1.2
 * @author Attila Kovacs
 */
int make_orbital_object(const char *name, long num, const novas_orbital *orbit, object *body) {
  static const char *fn = "make_orbital_object";
  if(!orbit)
    return novas_error(-1, EINVAL, fn, "Input orbital elements is NULL");
  prop_error(fn, (make_object(NOVAS_ORBITAL_OBJECT, num, name, NULL, body) ? -1 : 0), 0);
  body->orbit = *orbit;
  return 0;
}

/**
 * Converts angular quantities for stars to vectors.
 *
 * NOTES:
 * <ol>
 * <li>The velocity returned should not be used for deriving spectroscopic radial velocity. It is
 * a measure of the perceived change of the stars position, not a true physical velocity.</li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 * <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 * </ol>
 *
 * @param star         Pointer to catalog entry structure containing ICRS catalog
 * @param[out] pos     [AU] Position vector, equatorial rectangular coordinates, It may be NULL if
 *                     not required.
 * @param[out] motion  [AU/day] Perceived motion of star, in equatorial rectangular
 *                     coordinates. It must be distinct from the pos output vector, and may be
 *                     NULL if not required. Note, that it is suitable only for calculating the
 *                     apparent 3D location of the star at a different time, and should not be
 *                     used as a measure of physical velocity, e.g. for spectroscopic radial
 *                     velocity determination.
 *
 * @return             0 if successful, or -1 if the star argument is NULL or the output vectors
 *                     are the same pointer.
 *
 * @sa make_cat_entry()
 * @sa novas_init_cat_entry()
 */
int starvectors(const cat_entry *restrict star, double *restrict pos, double *restrict motion) {
  static const char *fn = "starvectors";

  double paralx;

  if(!star)
    return novas_error(-1, EINVAL, fn, "NULL input cat_entry");

  if(pos == motion)
    return novas_error(-1, EINVAL, fn, "identical output pos and vel 3-vectors @ %p", pos, motion);

  // If parallax is unknown, undetermined, or zero, set it to 1e-6
  // milliarcsecond, corresponding to a distance of 1 gigaparsec.
  paralx = star->parallax;
  if(star->parallax <= 0.0)
    paralx = 1.0e-6;

  // Convert right ascension, declination, and parallax to position vector
  // in equatorial system with units of AU.
  if(pos)
    radec2vector(star->ra, star->dec, 1.0 / sin(paralx * MAS), pos);

  if(motion) {
    // Compute Doppler factor, which accounts for change in
    // light travel time to star.
    const double k = 1.0 / (1.0 - star->radialvelocity * NOVAS_KMS / C);

    // Convert proper motion and radial velocity to orthogonal components of
    // motion with units of AU/day.
    motion[0] = k * star->promora / (paralx * JULIAN_YEAR_DAYS);
    motion[1] = k * star->promodec / (paralx * JULIAN_YEAR_DAYS);
    motion[2] = k * star->radialvelocity * NOVAS_KMS / (AU / DAY);

    // Transform motion vector to equatorial system.
    novas_los_to_xyz(motion, 15.0 * star->ra, star->dec, motion);
  }

  return 0;
}

/**
 * Returns the NOVAS planet ID for a given name (case insensitive), or -1 if no match is found.
 *
 * @param name    The planet name, or that for the "Sun", "Moon" or "SSB" (case insensitive).
 *                The spelled out "Solar System Barycenter" is also recognized with either spaces,
 *                hyphens ('-') or underscores ('_') separating the case insensitive words.
 * @return        The NOVAS major planet ID, or -1 (errno set to EINVAL) if the input name is
 *                NULL or if there is no match for the name provided.
 *
 * @author Attila Kovacs
 * @since 1.2
 *
 * @sa make_planet()
 */
enum novas_planet novas_planet_for_name(const char *restrict name) {
  static const char *fn = "novas_planet_for_name()";
  static const char *names[] = NOVAS_PLANET_NAMES_INIT;

  char *tok;
  int i;

  if(!name)
    return novas_error(-1, EINVAL, fn, "Input name is NULL");

  if(!name[0])
    return novas_error(-1, EINVAL, fn, "Input name is empty");

  for(i = 0; i < NOVAS_PLANETS; i++)
    if(strcasecmp(name, (const char *) names[i]) == 0)
      return i;

  // Check for Solar System Barycenter (and variants)
  tok = strtok(strdup(name), " \t-_");
  if(strcasecmp("solar", tok) == 0) {
    tok = strtok(NULL, " \t-_");
    if(tok && strcasecmp("system", tok) == 0) {
      tok = strtok(NULL, " \t-_");
      if(tok && strcasecmp("barycenter", tok) == 0)
        return NOVAS_SSB;
    }
  }

  return novas_error(-1, EINVAL, fn, "No match for name: '%s'", name);
}

/**
 * Transform a star's catalog quantities for a change the coordinate system and/or the date for
 * which the positions are calculated.  Also used to rotate catalog quantities on the dynamical
 * equator and equinox of J2000.0 to the ICRS or vice versa.
 *
 * 'date_incat' and 'date_newcat' may be specified either as a Julian date (e.g., 2433282.5 or
 * NOVAS_JD_B1950) or a fractional Julian year and fraction (e.g., 1950.0). Values less than 10000
 * are assumed to be years. You can also use the supplied constants NOVAS_JD_J2000 or
 * NOVAS_JD_B1950. The date arguments are ignored for the ICRS frame conversion options.
 *
 * If 'option' is PROPER_MOTION (1), input data can be in any reference system. If 'option' is
 * PRECESSION (2) or CHANGE_EPOCH (3), input data is assume to be in the dynamical system of
 * 'date_incat' and produces output in the dynamical system of 'date_outcat'. If 'option' is
 * CHANGE_J2000_TO_ICRS (4), the input data should be in the J2000.0 dynamical frame. And if
 * 'option' is CHANGE_ICRS_TO_J2000 (5), the input data must be in the ICRS, and the output will
 * be in the J2000 dynamical frame.
 *
 * This function cannot be properly used to bring data from old star catalogs into the modern
 * system, because old catalogs were compiled using a set of constants that are incompatible with
 * modern values.  In particular, it should not be used for catalogs whose positions and proper
 * motions were derived by assuming a precession constant significantly different from the value
 * implicit in function precession().
 *
 * @param option      Type of transformation
 * @param jd_tt_in    [day|yr] Terrestrial Time (TT) based Julian date, or year, of input catalog
 *                    data. Not used if option is CHANGE_J2000_TO_ICRS (4) or CHANGE_ICRS_TO_J2000
 *                    (5).
 * @param in          An entry from the input catalog, with units as given in the struct
 *                    definition
 * @param jd_tt_out   [day|yr] Terrestrial Time (TT) based Julian date, or year, of output catalog
 *                    data. Not used if option is CHANGE_J2000_TO_ICRS (4) or CHANGE_ICRS_TO_J2000
 *                    (5).
 * @param out_id      Catalog identifier (0 terminated). It may also be NULL in which case the
 *                    catalog name is inherited from the input.
 * @param[out] out    The transformed catalog entry, with units as given in the struct definition.
 *                    It may be the same as the input.
 * @return            0 if successful, -1 if either vector argument is NULL or if the 'option' is
 *                    invalid, or else 2 if 'out_id' is too long.
 *
 * @sa transform_hip()
 * @sa make_cat_entry()
 * @sa novas_epoch()
 * @sa NOVAS_JD_J2000
 * @sa NOVAS_JD_B1950
 * @sa NOVAS_JD_HIP
 */
short transform_cat(enum novas_transform_type option, double jd_tt_in, const cat_entry *in, double jd_tt_out, const char *out_id,
        cat_entry *out) {
  static const char *fn = "transform_cat";

  double paralx, k;
  double pos[3], vel[3], term1, xyproj;
  double djd = (jd_tt_out - jd_tt_in);

  if(!in || !out)
    return novas_error(-1, EINVAL, fn, "NULL parameter: in=%p, out=%p", in, out);

  if(out_id && strlen(out_id) >= SIZE_OF_CAT_NAME)
    return novas_error(2, EINVAL, fn, "output catalog ID is too long (%d > %d)", (int) strlen(out_id), SIZE_OF_CAT_NAME - 1);

  if(option == CHANGE_J2000_TO_ICRS || option == CHANGE_ICRS_TO_J2000) {
    // ICRS frame ties always assume J2000 for both input and output...
    jd_tt_in = NOVAS_JD_J2000;
    jd_tt_out = NOVAS_JD_J2000;
  }
  else {
    // If necessary, compute Julian dates.

    // This function uses TDB Julian dates internally, but no distinction between TDB and TT is necessary.
    if(jd_tt_in < 10000.0)
      jd_tt_in = JD_J2000 + (jd_tt_in - 2000.0) * JULIAN_YEAR_DAYS;
    if(jd_tt_out < 10000.0)
      jd_tt_out = JD_J2000 + (jd_tt_out - 2000.0) * JULIAN_YEAR_DAYS;
  }

  // Convert input angular components to vectors

  // If parallax is unknown, undetermined, or zero, set it to 1.0e-6
  // milliarcsecond, corresponding to a distance of 1 gigaparsec.
  paralx = in->parallax;
  if(paralx <= 0.0)
    paralx = 1.0e-6;

  // Convert right ascension, declination, and parallax to position
  // vector in equatorial system with units of AU.
  radec2vector(in->ra, in->dec, 1.0 / sin(paralx * MAS), pos);

  // Compute Doppler factor, which accounts for change in light travel time to star.
  k = 1.0 / (1.0 - in->radialvelocity * NOVAS_KMS / C);

  // Convert proper motion and radial velocity to orthogonal components
  // of motion, in spherical polar system at star's original position,
  // with units of AU/day.
  term1 = paralx * JULIAN_YEAR_DAYS;
  vel[0] = k * in->promora / term1;
  vel[1] = k * in->promodec / term1;
  vel[2] = k * in->radialvelocity * DAY / AU_KM;

  // Transform motion vector to equatorial system.
  novas_los_to_xyz(vel, 15.0 * in->ra, in->dec, vel);

  // Update star's position vector for space motion (only if 'option' = 1 or 'option' = 3).
  if((option == PROPER_MOTION) || (option == CHANGE_EPOCH)) {
    int j;
    for(j = 0; j < 3; j++)
      pos[j] += vel[j] * djd;
  }

  switch(option) {
    case PROPER_MOTION:
      break;

    case PRECESSION:
    case CHANGE_EPOCH: {
      // Precess position and velocity vectors (only if 'option' = 2 or 'option' = 3).
      prop_error("transform_cat", precession(jd_tt_in, pos, jd_tt_out, pos), 0);
      precession(jd_tt_in, vel, jd_tt_out, vel);
      break;
    }

    case CHANGE_J2000_TO_ICRS:
      // Rotate dynamical J2000.0 position and velocity vectors to ICRS (only if 'option' = 4).
      frame_tie(pos, J2000_TO_ICRS, pos);
      frame_tie(vel, J2000_TO_ICRS, vel);
      break;

    case CHANGE_ICRS_TO_J2000:
      // Rotate ICRS position and velocity vectors to dynamical J2000.0 (only if 'option' = 5).
      frame_tie(pos, ICRS_TO_J2000, pos);
      frame_tie(vel, ICRS_TO_J2000, vel);
      break;

    default:
      if(out != in)
        *out = *in;

      return novas_error(-1, EINVAL, fn, "invalid option %d", option);
  }

  // Convert vectors back to angular components for output.

  // From updated position vector, obtain star's new position expressed as angular quantities.
  xyproj = sqrt(pos[0] * pos[0] + pos[1] * pos[1]);

  out->ra = (xyproj > 0.0) ? atan2(pos[1], pos[0]) / HOURANGLE : 0.0;
  if(out->ra < 0.0)
    out->ra += DAY_HOURS;

  out->dec = atan2(pos[2], xyproj) / DEGREE;

  paralx = asin(1.0 / novas_vlen(pos)) / MAS;

  // Transform motion vector back to spherical polar system at star's new position.
  novas_xyz_to_los(vel, 15.0 * out->ra, out->dec, vel);

  // Convert components of motion to from AU/day to normal catalog units.
  out->promora = vel[0] * term1 / k;
  out->promodec = vel[1] * term1 / k;
  out->radialvelocity = vel[2] * (AU_KM / DAY) / k;
  out->parallax = (in->parallax > 0.0) ? paralx : 0.0;

  // Set the catalog identification code for the transformed catalog entry.
  if(out_id)
    strncpy(out->catalog, out_id, SIZE_OF_CAT_NAME - 1);
  else if(out != in)
    strncpy(out->catalog, in->catalog, SIZE_OF_CAT_NAME - 1);
  // Make sure catalog name is terminated.
  out->catalog[SIZE_OF_CAT_NAME - 1] = '\0';

  if(out != in) {
    // Copy unchanged quantities from the input catalog entry to the transformed catalog entry.
    strncpy(out->starname, in->starname, SIZE_OF_OBJ_NAME - 1);
    out->starname[SIZE_OF_OBJ_NAME - 1] = '\0';
    out->starnumber = in->starnumber;
  }

  return 0;
}

/**
 * Convert Hipparcos catalog data at epoch J1991.25 to epoch J2000.0, for use within NOVAS.
 * To be used only for Hipparcos or Tycho stars with linear space motion.  Both input and
 * output data is in the ICRS.
 *
 * @param hipparcos       An entry from the Hipparcos catalog, at epoch J1991.25, with 'ra' in
 *                        degrees(!) as per Hipparcos catalog units.
 * @param[out] hip_2000   The transformed input entry, at epoch J2000.0, with 'ra' in hours(!)
 *                        as per the NOVAS convention. It may be the same as the input.
 *
 * @return            0 if successful, or -1 if either of the input pointer arguments is NULL.
 *
 * @sa make_cat_entry()
 * @sa NOVAS_JD_HIP
 */
int transform_hip(const cat_entry *hipparcos, cat_entry *hip_2000) {
  static const char *fn = "transform_hip";
  cat_entry scratch;

  if(!hipparcos)
    return novas_error(-1, EINVAL, fn, "NULL Hipparcos input catalog entry");

  // Set up a "scratch" catalog entry containing Hipparcos data in
  // "NOVAS units."
  scratch = *hipparcos;
  strcpy(scratch.catalog, "SCR");

  // Convert right ascension from degrees to hours.
  scratch.ra /= 15.0;

  // Change the epoch of the Hipparcos data from J1991.25 to J2000.0.
  prop_error(fn, transform_cat(1, NOVAS_JD_HIP, &scratch, JD_J2000, "HP2", hip_2000), 0);
  return 0;
}

/**
 * Applies proper motion, including foreshortening effects, to a star's position.
 *
 * REFERENCES:
 * <ol>
 *  <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 * </ol>
 *
 * @param jd_tdb_in  [day] Barycentric Dynamical Time (TDB) based Julian date of the first epoch.
 * @param pos        [AU] Position vector at first epoch.
 * @param vel        [AU/day] Velocity vector at first epoch.
 * @param jd_tdb_out [day] Barycentric Dynamical Time (TDB) based Julian date of the second epoch.
 * @param[out] out   Position vector at second epoch. It can be the same vector as the input.
 * @return           0 if successful, or -1 if any of the vector areguments is NULL.
 *
 * @sa transform_cat()
 */
int proper_motion(double jd_tdb_in, const double *pos, const double *restrict vel, double jd_tdb_out, double *out) {
  const double dt = jd_tdb_out - jd_tdb_in;
  int j;

  if(!pos || !vel || !out)
    return novas_error(-1, EINVAL, "proper_motion", "NULL input or output 3-vector: pos=%p, vel=%p, out=%p", pos, vel, out);

  for(j = 3; --j >= 0;)
    out[j] = pos[j] + vel[j] * dt;

  return 0;
}

/**
 * Returns a Solar-system body's distance from the Sun, and optionally also the rate of recession.
 * It may be useful, e.g. to calculate the body's heating from the Sun.
 *
 * @param jd_tdb      [day] Barycentric Dynamical Time (TDB) based Julian date. You may want to
 *                    use a time that is antedated to when the observed light originated from the
 *                    source.
 * @param source      Observed Solar-system source
 * @param[out] rate   [AU/day] (optional) Returned rate of recession from Sun
 * @return            [AU] Distance from the Sun, or NAN if not a Solar-system source.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_solar_power()
 * @sa novas_solar_illum()
 */
double novas_helio_dist(double jd_tdb, const object *restrict source, double *restrict rate) {
  static const char *fn = "novas_helio_dist";

  const double jd2[2] = { jd_tdb, 0.0 };
  double pos[3], vel[3], d;

  if(rate)
    *rate = NAN;

  if(!source) {
    novas_set_errno(EINVAL, fn, "input source is NULL");
    return NAN;
  }

  if(source->type == NOVAS_CATALOG_OBJECT) {
    novas_set_errno(EINVAL, fn, "input source is not a Solar-system body: type %d", source->type);
    return NAN;
  }

  if(ephemeris(jd2, source, NOVAS_HELIOCENTER, NOVAS_REDUCED_ACCURACY, pos, vel) != 0)
    return novas_trace_nan(fn);

  d = novas_vlen(pos);
  if(!d) {
    // The Sun itself...
    if(rate)
      *rate = 0.0;
    return 0.0;
  }

  if(rate)
    *rate = novas_vlen(vel);

  return d;
}

/**
 * Returns the typical incident Solar power on a Solar-system body at the time of observation.
 *
 * @param jd_tdb  [day] Barycentric Dynamical Time (TDB) based Julian date. You may want to
 *                use a time that is antedated to when the observed light originated (
 *                was reflected) from the source.
 * @param source  Observed Solar-system source
 * @return        [W/m<sup>2</sup>] Incident Solar power on the illuminated side of the object,
 *                or NAN if not a Solar-system source or if the source is the Sun itself.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_solar_illum()
 */
double novas_solar_power(double jd_tdb, const object *restrict source) {
  double d = novas_helio_dist(jd_tdb, source, NULL);
  return NOVAS_SOLAR_CONSTANT / (d * d);
}

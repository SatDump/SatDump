/**
 * @file
 *
 * @author A. Kovacs
 * @since 1.2
 *
 *  SuperNOVAS Solar-system ephemeris lookup implementation via the CALCEPH C library.
 *
 *  This is an optional component of SuperNOVAS, which interfaces to the CALCEPH C library. As
 *  such, you may need the CALCEPH runtime libraries in an accessible location (such as in
 *  `/usr/lib`) to use, and you will need development files (C headers and unversioned libraries)
 *  to build. Thus, this module is compiled only if `CALCEPH_SUPPORT` is set to 1 prior to the
 *  build.
 *
 *  To use, simply include `novas-calceph.h` in your application source, configure a
 *  `t_calcephbin` object with the requisite ephemeris data, and then call `novas_use_calceph()`
 *  with it to activate. E.g.:
 *
 *  ```c
 *    #include <novas.h>
 *    #include <novas-calceph.h>
 *
 *    // You can open a set of JPL/INPOP ephemeris files with CALCEPH...
 *    t_calcephbin *eph = calceph_open_array(...);
 *
 *    // Then use them as your generic SuperNOVAS ephemeris provider
 *    int status = novas_use_calceph(eph);
 *    if(status < 0) {
 *      // Ooops something went wrong...
 *    }
 *  ```
 *
 * Optionally, you may use a separate ephemeris dataset for major planets (or if planet
 * ephemeris was included in 'eph' above, you don't have to):
 *
 *  ```c
 *    t_calcephbin *pleph = calceph_open(...);
 *    status = novas_use_calceph_planets(pleph);
 *    if(status < 0) {
 *      // Ooops something went wrong...
 *    }
 *  ```
 *
 * By default the CALCEPH plugin will use NAIF ID numbers for the lookup (for planets the
 * NOVAS IDs will be mapped to NAIF IDs automatically). You can enable name-based lookup by
 * setting the @ref object number to -1 (e.g. in `make_ephem_object()`), or else switch to
 * using CALCEPH IDs by calling `novas_calceph_use_ids(NOVAS_ID_CALCEPH)`.
 *
 * REFERENCES:
 * <ol>
 *  <li>CALCEPH is at https://calceph.imcce.fr/docs/4.0.0/html/c/</li>
 *  <li>CALCEPH source code is at https://gitlab.obspm.fr/imcce_calceph/calceph</li>
 * </ol>
 *
 * @sa solarsystem.h
 * @sa solsys-cspice.c
 */

#include <string.h>
#include <errno.h>
#include <semaphore.h>

/// \cond PRIVATE
#define __NOVAS_INTERNAL_API__      ///< Use definitions meant for internal use by SuperNOVAS only
#include "novas.h"
#include "novas-calceph.h"

#define CALCEPH_MOON                10  ///< Moon in CALCEPH
#define CALCEPH_SUN                 11  ///< Sun in CALCEPH
#define CALCEPH_SSB                 12  ///< Solar-system Barycenter in CALCEPH

/// Distance and time units to use for CALCEPH (AU would be conventient, but is not available
/// unless defined in the sphemeris file(s) themselves.
#define CALCEPH_UNITS               (CALCEPH_UNIT_KM | CALCEPH_UNIT_DAY)

/// Multiplicative normalization for the positions returned by CALCEPH to AU
#define NORM_POS                    (NOVAS_KM / NOVAS_AU)

/// Multiplicative normalization for the velocities returned by CALCEPH to AU/day
#define NORM_VEL                    (NORM_POS)

/// Whether to force serialized (non-parallel CALCEPH queries)
int serialized_calceph_queries;

/// \endcond

static int compute_flags = CALCEPH_USE_NAIFID;

/// CALCEPH ephemeris specifically for planets (and Sun and Moon) only
static t_calcephbin *planets;

/// (boolean) whether the planets ephemeris data is thread safe to access
static int is_thread_safe_planets;

/// Semaphore for thread-safe access of planet ephemeris (if needed)
static sem_t sem_planets;

/// Generic CALCEPH ephemeris files for all types of Solar-system sources
static t_calcephbin *bodies;

/// (boolean) whether the generic solar-system bodies ephemeris data is thread safe to access
static int is_thread_safe_bodies;

/// Semaphore for thread-safe access of generic solar-system bodies ephemeris (if needed)
static sem_t sem_bodies;


static int mutex_lock(sem_t *sem) {
  if(sem_wait(sem) != 0)
    return novas_error(-1, errno, "mutex_lock()", "sem_wait()");
  return 0;
}

static int mutex_unlock(sem_t *sem) {
  sem_post(sem);
  return 0;
}

static int prep_ephem(t_calcephbin *eph) {
  static const char *fn = "prep_ephem";

  if(!eph)
    return novas_error(-1, EINVAL, fn, "input ephemeris data is NULL");

  if(!calceph_prefetch(eph))
    return novas_error(-1, EAGAIN, fn, "calceph_prefetch() failed");

  return 0;
}

/**
 * Sets the type of Solar-system body IDs to use as object.number with NOVAS_EPHEM_OBJECT types.
 * CALCEPH supports the use of both NAIF and its own numbering system to identify Solar-system
 * bodies. So, this function gives you the choice on which numbering system you want to use
 * in object data structures. The choice does not affect major planets (which always use the
 * NOVAS numbering scheme), or catalog objects.
 *
 * @param idtype    NOVAS_ID_NAIF to use NAIF IDs (default) or else NOVAS_ID_CALCEPH to use
 *                  the CALCEPH body numbering convention for object.
 * @return          0 if successful or else -1 (errno set to EINVAL) if the input value is
 *                  invalid.
 *
 * @sa make_planet(), make_ephem_object(), NOVAS_EPHEM_OBJECT
 */
int novas_calceph_use_ids(enum novas_id_type idtype) {
  switch(idtype) {
    case NOVAS_ID_NAIF:
      compute_flags = CALCEPH_USE_NAIFID;
      return 0;
    case NOVAS_ID_CALCEPH:
      compute_flags = 0;
      return 0;
    default:
      return novas_error(-1, EINVAL, "novas_calceph_use_ids", "Invalid body ID: %d\n", idtype);
  }
}


/**
 * Provides an interface between the CALCEPH C library and NOVAS-C for regular (reduced) precision
 * applications. The user must set the CALCEPH ephemeris binary data to use using the
 * novas_use_calceph() or novas_use_calceph_planet() to activate the desired CALCEPH ephemeris
 * data prior to use.
 *
 * This call is always thread safe, even when CALCEPH and the ephemeris data may not be. When
 * necessary, the ephemeris access will be mutexed to ensure sequential access under the hood.
 *
 * REFERENCES:
 * <ol>
 *  <li>The CALCEPH C library; https://calceph.imcce.fr/docs/4.0.0/html/c/index.html</li>
 *  <li>Kaplan, G. H. "NOVAS: Naval Observatory Vector Astrometry
 *  Subroutines"; USNO internal document dated 20 Oct 1988;
 *  revised 15 Mar 1990.</li>
 * </ol>
 *
 * @param jd_tdb         [day] Two-element array containing the Julian date, which may be split
 *                       any way (although the first element is usually the "integer" part, and
 *                       the second element is the "fractional" part).  Julian date is on the TDB
 *                       or "T_eph" time scale.
 * @param body           Major planet number (or that for Sun, Moon, SSB...)
 * @param origin         NOVAS_BARYCENTER (0) or NOVAS_HELIOCENTER (1)
 *                       -- relative to which to report positions and velocities.
 * @param[out] position  [AU] Position vector of 'body' at jd_tdb; equatorial rectangular
 *                       coordinates in AU referred to the ICRS.
 * @param[out] velocity  [AU/day] Velocity vector of 'body' at jd_tdb; equatorial rectangular
 *                       system referred to the ICRS, in AU/day.
 * @return               0 if successful, or else 1 if the 'body' is invalid, or 2 if the
 *                       'origin' is invalid, or 3 if there was an error providing ephemeris
 *                       data.
 *
 * @author Attila Kovacs
 * @since 1.2
 *
 * @sa planet_calceph(), novas_use_calceph(), novas_use_calceph_planet()
 */
static short planet_calceph_hp(const double jd_tdb[restrict 2], enum novas_planet body, enum novas_origin origin,
        double *restrict position, double *restrict velocity) {
  static const char *fn = "planet_calceph_hp";

  sem_t *sem = (planets == bodies) ? &sem_bodies : &sem_planets;
  const int lock = !is_thread_safe_planets || serialized_calceph_queries;
  double pv[6] = {0.0};
  int i, target, center, success;

  if(!jd_tdb)
    return novas_error(-1, EINVAL, fn, "jd_tdb input time array is NULL.");

  switch(body) {
    case NOVAS_SSB:
      target = CALCEPH_SSB;
      break;
    case NOVAS_SUN:
      target = CALCEPH_SUN;
      break;
    case NOVAS_MOON:
      target = CALCEPH_MOON;
      break;
    default:
      if (body < NOVAS_MERCURY || body > NOVAS_PLUTO)
        return novas_error(1, EINVAL, fn, "Invalid major planet: %d", body);
      target = body;
  }

  switch(origin) {
    case NOVAS_BARYCENTER:
      center = CALCEPH_SSB;
      break;
    case NOVAS_HELIOCENTER:
      center = CALCEPH_SUN;
      break;
    default:
      return novas_error(2, EINVAL, fn, "Invalid origin type: %d", origin);
  }

  if(lock)
    prop_error(fn, mutex_lock(sem), 0);

  success = calceph_compute_unit(planets, jd_tdb[0], jd_tdb[1], target, center, CALCEPH_UNITS, pv);

  if(lock)
   mutex_unlock(sem);

  if(!success)
    return novas_error(3, EAGAIN, fn, "calceph_compute() failure (NOVAS ID=%d)", body);

  for(i = 3; --i >= 0;) {
    if(position)
      position[i] = pv[i] * NORM_POS;
    if(velocity)
      velocity[i] = pv[3 + i] * NORM_VEL;
  }

  return 0;
}

/**
 * Provides an interface between the CALCEPH C library and NOVAS-C for regular (reduced) precision
 * applications. The user must set the CALCEPH ephemeris binary data to use using the
 * novas_use_calceph() or novas_use_calceph_planet() to activate the desired CALCEPH ephemeris
 * data prior to use.
 *
 * This call is always thread safe, even when CALCEPH and the ephemeris data may not be. When
 * necessary, the ephemeris access will be mutexed to ensure sequential access under the hood.
 *
 * REFERENCES:
 * <ol>
 *  <li>The CALCEPH C library; https://calceph.imcce.fr/docs/4.0.0/html/c/index.html</li>
 *  <li>Kaplan, G. H. "NOVAS: Naval Observatory Vector Astrometry
 *  Subroutines"; USNO internal document dated 20 Oct 1988;
 *  revised 15 Mar 1990.</li>
 * </ol>
 *
 * @param jd_tdb         [day] Two-element array containing the Julian date, which may be split
 *                       any way (although the first element is usually the "integer" part, and
 *                       the second element is the "fractional" part).  Julian date is on the TDB
 *                       or "T_eph" time scale.
 * @param body           Major planet number (or that for Sun, Moon, SSB...)
 * @param origin         NOVAS_BARYCENTER (0) or NOVAS_HELIOCENTER (1), or 2 for Earth geocenter
 *                       -- relative to which to report positions and velocities.
 * @param[out] position  [AU] Position vector of 'body' at jd_tdb; equatorial rectangular
 *                       coordinates in AU referred to the ICRS.
 * @param[out] velocity  [AU/day] Velocity vector of 'body' at jd_tdb; equatorial rectangular
 *                       system referred to the ICRS, in AU/day.
 * @return               0 if successful, or else an error code of solarsystem().
 *
 * @author Attila Kovacs
 * @since 1.2
 *
 * @sa set_planet_provider(), solarsystem()
 */
static short planet_calceph(double jd_tdb, enum novas_planet body, enum novas_origin origin,
        double *restrict position, double *restrict velocity) {
  const double tjd[2] = { jd_tdb, 0.0 };

  prop_error("planet_calceph", planet_calceph_hp(tjd, body, origin, position, velocity), 0);
  return 0;
}

/**
 * Generic ephemeris handling via the CALCEPH C library. This call is always thread safe, even
 * when CALCEPH and the ephemeris data may not be. When necessary, the ephemeris access will be
 * mutexed to ensure sequential access under the hood.
 *
 * @param name          The name of the solar-system body. It is important only if the 'id' is
 *                      -1.
 * @param id            The NAIF or CALCEPH ID number of the solar-system body for which the
 *                      position in desired, or -1 if the 'name' should be used instead to
 *                      identify the object.
 * @param jd_tdb_high   [day] The high-order part of Barycentric Dynamical Time (TDB) based
 *                      Julian date for which to find the position and velocity. Typically
 *                      this may be the integer part of the Julian date for high-precision
 *                      calculations, or else the entire Julian date for reduced precision.
 * @param jd_tdb_low    [day] The low-order part of Barycentric Dynamical Time (TDB) based
 *                      Julian date for which to find the position and velocity. Typically
 *                      this may be the fractional part of the Julian date for high-precision
 *                      calculations, or else 0.0 if the date is defined entirely by the
 *                      high-order component for reduced precision.
 * @param[out] origin   Set to NOVAS_BARYCENTER or NOVAS_HELIOCENTER to indicate relative to
 *                      which the ephemeris positions/velocities are reported.
 * @param[out] pos      [AU] position 3-vector to populate with rectangular equatorial
 *                      coordinates in AU. It may be NULL if position is not required.
 * @param[out] vel      [AU/day] velocity 3-vector to populate in rectangular equatorial
 *                      coordinates in AU/day. It may be NULL if velocities are not required.
 * @return              0 if successful, -1 if any of the pointer arguments are NULL, or some
 *                      non-zero value if the was an error s.t. the position and velocity
 *                      vector should not be used.
 *
 * @author Attila Kovacs
 * @since 1.2
 *
 * @sa novas_calceph_use_ids(), make_ephem_object(), set_ephem_provider(), ephemeris()
 */
static int novas_calceph(const char *name, long id, double jd_tdb_high, double jd_tdb_low, enum novas_origin *origin, double *pos, double *vel) {
  static const char *fn = "novas_calceph";

  double pv[6] = {0.0};
  const int lock = !is_thread_safe_bodies || serialized_calceph_queries;
  int i, success, center;

  if(id == -1) {
    // Lookup by name...

    if(!name)
      return novas_error(-1, EINVAL, fn, "id=-1 and name is NULL");

    if(!name[0])
      return novas_error(-1, EINVAL, fn, "id=-1 and name is empty");

    // Use name to get NAIF ID.
    if(!calceph_getidbyname(bodies, name, compute_flags, &i))
      return novas_error(1, EINVAL, fn, "CALCEPH could not find a NAIF ID for '%s'", name);

    id = i;
  }

  // Always return positions and velocities w.r.t. the SSB
  if(origin)
    *origin = NOVAS_BARYCENTER;

  center = (compute_flags & CALCEPH_USE_NAIFID) ? NAIF_SSB : CALCEPH_SSB;

  if(lock)
    prop_error(fn, mutex_lock(&sem_bodies), 0);

  success = calceph_compute_unit(bodies, jd_tdb_high, jd_tdb_low, id, center, (compute_flags | CALCEPH_UNITS), pv);

  if(lock)
    mutex_unlock(&sem_bodies);

  if(!success)
    return novas_error(3, EAGAIN, fn, "calceph_compute() failure (name='%s', NAIF=%ld)", name ? name : "<null>", id);

  for(i = 3; --i >= 0;) {
    if(pos)
      pos[i] = pv[i] * NORM_POS;
    if(vel)
      vel[i] = pv[3 + i] * NORM_VEL;
  }

  return 0;
}

/**
 * Sets a ephemeris provider for Solar-system objects using the CALCEPH C library and the
 * specified set of ephemeris files. If the supplied ephemeris files contain data for major
 * planets also, they can be used by planet_calceph() / planet_calceph_hp() also, unless a
 * separate CALCEPH ephemeris data is set via novas_use_calceph_planets().
 *
 * The call also make CALCEPH the default ephemeris provider for all types of Solar-system
 * objects. If you want to use another provider for major planets, you need to call
 * set_planet_provider() / set_planet_provider_hp() afterwards to specify a different provider for
 * the major planets (and Sun, Moon, SSB...).
 *
 * @param eph   Pointer to the CALCEPH ephemeris data that have been opened.
 * @return      0 if successful, or else -1 (errno will indicate the type of error).
 *
 * @sa novas_calceph_use_ids(), novas_use_calceph_planets(), set_ephem_provider()
 *
 * @author Attila Kovacs
 * @since 1.2
 */
int novas_use_calceph(t_calcephbin *eph) {
  static const char *fn = "novas_use_calceph";

  prop_error(fn, prep_ephem(eph), 0);

  // If first time, then initialize the bodies semaphore
  if(!bodies)
    sem_init(&sem_bodies, 0, 1);

  // Make sure we don't change the ephemeris provider while using it
  prop_error(fn, mutex_lock(&sem_bodies), 0);

  is_thread_safe_bodies = calceph_isthreadsafe(eph);
  bodies = eph;
  mutex_unlock(&sem_bodies);

  // Use CALCEPH as the default minor body ephemeris provider
  set_ephem_provider(novas_calceph);

  // If no planet provider is set (yet) use the same ephemeris for planets too
  // atleast until a dedicated planet provider is set.
  if (!planets) novas_use_calceph_planets(eph);

  return 0;
}

/**
 * Sets the CALCEPH C library and the specified ephemeris data as the ephemeris provider for the
 * major planets (and Sun, Moon, SSB...).
 *
 * @param eph   Pointer to the CALCEPH ephemeris data for the major planets (including Sun, Moon,
 *              SSB...) that have been opened.
 * @return      0 if successful, or else -1 (errno will indicate the type of error).
 *
 * @author Attila Kovacs
 * @since 1.2
 *
 * @sa novas_use_calceph(), set_planet_provider(), set_planet_provider_hp()
 */
int novas_use_calceph_planets(t_calcephbin *eph) {
  static const char *fn = "novas_use_calceph_planets";

  prop_error(fn, prep_ephem(eph), 0);

  // If first time, then initialize the planet semaphore
  if(!planets)
    sem_init(&sem_planets, 0, 1);

  // Make sure we don't change the ephemeris provider while using it
  prop_error(fn, mutex_lock(&sem_planets), 0);

  is_thread_safe_planets = calceph_isthreadsafe(eph);
  planets = eph;
  mutex_unlock(&sem_planets);

  // Use calceph as the default NOVAS planet provider
  set_planet_provider_hp(planet_calceph_hp);
  set_planet_provider(planet_calceph);

  return 0;
}


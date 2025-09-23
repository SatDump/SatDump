/**
 * @file
 *
 * @date Created  on Mar 6, 2025
 * @author Attila Kovacs
 *
 *  Function relating to the use of Keplerian orbital elements. For example, the IAU Minor Planet
 *  Center publishes up-to-date Keplerial orbital elements for all asteroids, comets, and
 *  Near-Earth Objects (NEOs) regularly. On short timescales these can provide accurate positions
 *  for such objects, that are as good as, or comparable to, the ephemeris data provided by NASA
 *  JPL.
 *
 *  For newly discovered objects, the MPC orbital elements may be the only source, or the most
 *  accurate source, of position information.
 *
 *  To use Keplerian orbital elements with __SuperNOVAS__, simply define the orbital parameters
 *  in a `novas_orbital` structure. By default heliocentric orbits are assumed, but you may also
 *  define orbitals around other bodies, such as a major planet, by defining the body (or
 *  barycenter) that is at the center of the orbital, and the orientation of the orbital system
 *  (w.r.t. the ecliptic or equator of date) with `novas_set_orbsys_pole()`. E.g.:
 *
 *  ```c
 *    novas_orbit orb = NOVAS_ORBIT_INIT;
 *
 *    orb.a = ...    // [AU] semi-major axis
 *    ...            // populate the rest of the orbital parameters.
 *
 *    // Set Jupiter as the orbital center
 *    orb.system.center = NOVAS_JUPITER_INIT;
 *
 *    // Set the orientation by defining the RA/Dec of the orbital pole, say in ICRS
 *    novas_set_orbsys_pole(NOVAS_ICRS, ra_pole, dec_pole, &orb.system);
 *  ```
 *
 *  Once the orbital is defined, you can use it to define a generic @ref object via
 *  `make_orbital_object()` to enable the the same astrometric calculations as for ephemeris
 *  sources, e.g.:
 *
 *  ```c
 *    object source;
 *
 *    // Let's say the orbital is that for Callisto, so we set the source name
 *    // and NAIF ID number (504) accordingly.
 *    make_orbital_object("Callisto", 504, &orb, &source);
 *  ```
 *
 *  Or, you can calculate geometric orbital positions and velocities directly via
 *  `novas_orbit_posvel()`, while `novas_orbit_native_posvel()` will do the same, but in
 *  the native coordinate system in which the orbital is defined.
 *
 *  @sa make_orbital_object()
 */

#include <errno.h>

/// \cond PRIVATE
#define __NOVAS_INTERNAL_API__    ///< Use definitions meant for internal use by SuperNOVAS only
#include "novas.h"
/// \endcond


/**
 * Change xzy vectors to the new polar orientation. &theta, &phi define the orientation of the
 * input pole in the output system.
 *
 * @param in        input 3-vector in the original system (pole = z)
 * @param theta     [deg] polar angle of original pole in the new system
 * @param phi       [deg] azimuthal angle of original pole in the new system
 * @param[out] out  output 3-vector in the new (rotated) system. It may be the same vector as the
 *                  input.
 * @return          0
 *
 */
static int change_pole(const double *in, double theta, double phi, double *out) {
  double x, y, z;
  double ca, sa, cb, sb;

  x = in[0];
  y = in[1];
  z = in[2];

  theta *= DEGREE;
  phi *= DEGREE;

  ca = cos(phi);
  sa = sin(phi);
  cb = cos(theta);
  sb = sin(theta);

  out[0] = ca * x - sa * (cb * y + sb * z);
  out[1] = sa * x + ca * (cb * y - sb * z);
  out[2] = sb * y + cb * z;

  return 0;
}

/**
 * Converts equatorial coordinates of a given type to GCRS equatorial coordinates
 *
 * @param jd_tdb    [day] Barycentric Dynamical Time (TDB) based Julian Date
 * @param sys       The type of equatorial system assumed for the input (ICRS / GCRS, J2000, TOD,
 *                  MOD, or CIRS). It must be an inertial celestial system, i.e. it cannot be a
 *                  reference system which co-rotates with Earth (like ITRS to TIRS).
 * @param[in, out] vec  vector to change to GCRS.
 * @return          0 if successful, or else -1 (errno set to EINVAL) if the 'sys' argument is
 *                  invalid or unsupported.
 *
 * @author Attila Kovacs
 * @since 1.2
 */
static int equ2gcrs(double jd_tdb, enum novas_reference_system sys, double *vec) {
  static const char *fn = "equ2gcrs";

  switch(sys) {
    case NOVAS_GCRS:
    case NOVAS_ICRS:
      return 0;
    case NOVAS_CIRS:
      return cirs_to_gcrs(jd_tdb, NOVAS_REDUCED_ACCURACY, vec, vec);
    case NOVAS_J2000:
      return j2000_to_gcrs(vec, vec);
    case NOVAS_TOD:
      return tod_to_gcrs(jd_tdb, NOVAS_REDUCED_ACCURACY, vec, vec);
    case NOVAS_MOD:
      return mod_to_gcrs(jd_tdb, vec, vec);
    default:
      return novas_error(-1, EINVAL, fn, "invalid orbital system type: %d", sys);
  }
}

/**
 * Convert coordinates in an orbital system to GCRS equatorial coordinates
 *
 * @param jd_tdb        [day] Barycentric Dynamic Time (TDB) based Julian Date
 * @param sys           Orbital system specification
 * @param accuracy      NOVAS_FULL_ACCURACY or NOVAS_REDUCED_ACCURACY
 * @param[in, out] vec  Coordinates
 * @return              0 if successful, or else an error from ecl2equ_vec().
 *
 * @author Attila Kovacs
 * @since 1.2
 */
static int orbit2gcrs(double jd_tdb, const novas_orbital_system *sys, enum novas_accuracy accuracy, double *vec) {
  static const char *fn = "orbit2gcrs";

  if(accuracy != NOVAS_FULL_ACCURACY && accuracy != NOVAS_REDUCED_ACCURACY)
    return novas_error(-1, EINVAL, fn, "invalid accuracy: %d", accuracy);

  if(sys->obl)
    change_pole(vec, sys->obl, sys->Omega, vec);

  if(sys->plane == NOVAS_ECLIPTIC_PLANE) {
    enum novas_equator_type eq;
    double jd = jd_tdb;

    switch(sys->type) {
      case NOVAS_GCRS:
      case NOVAS_ICRS:
        eq = NOVAS_GCRS_EQUATOR;
        jd = NOVAS_JD_J2000;
        break;
      case NOVAS_J2000:
        eq = NOVAS_TRUE_EQUATOR;
        jd = NOVAS_JD_J2000;
        break;
      case NOVAS_TOD:
      case NOVAS_CIRS:
        eq = NOVAS_TRUE_EQUATOR;
        break;
      case NOVAS_MOD:
        eq = NOVAS_MEAN_EQUATOR;
        break;
      default:
        return novas_error(-1, EINVAL, fn, "invalid reference system: %d", sys->type);
    }

    ecl2equ_vec(jd, eq, accuracy, vec, vec);
  }
  else if(sys->plane != NOVAS_EQUATORIAL_PLANE)
    return novas_error(-1, EINVAL, fn, "invalid orbital system reference plane type: %d", sys->type);

  prop_error(fn, equ2gcrs(jd_tdb, sys->type, vec), 0);

  return 0;
}

/**
 * Calculates the eccentric anomaly, relative radius (w.r.t. the semi-major axis _a_), and true
 * elongation of a Keplerian orbital in the plane of the orbit.
 *
 * @param M           [deg] The mean elongation (as if on a circular orbit).
 * @param e           eccentricity.
 * @param[out] E      [deg] The solved eccentric anomaly.
 * @param[out] r_hat  Relative distance from orbital center, as a multiple of the semi-major axis
 *                    _a_.
 * @param[out] l      [deg] The true elongation along the orbit.
 * @return            0 if successful, or else -1 if there was an error (errno will be set to
 *                    ECANCELED if the iterative solution to the eccentric anomaly _E_ failed to
 *                    converge).
 *
 * @since 1.4
 * @author Attila Kovacs
 *
 * @sa novas_orbit_posvel()
 */
static int novas_orbital_plane_pos(double M, double e, double *restrict E, double *restrict r_hat, double *restrict l) {
  double E1 = remainder(M * DEGREE, TWOPI);
  int i = novas_inv_max_iter;

  M = E1; // deg -> rad

  // Iteratively determine E, using Newton-Raphson method...
  while(--i >= 0) {
    double dE = (E1 - e * sin(E1) - M) / (1.0 - e * cos(E1));
    E1 -= dE;
    if(fabs(dE) < EPREC)
      break;
  }

  if(i < 0)
    return novas_error(-1, ECANCELED, "novas_orbital_plane_pos", "Eccentric anomaly convergence failure.");

  *E = remainder(E1, TWOPI) / DEGREE;
  *l = 2.0 * atan2(sqrt(1.0 + e) * sin(0.5 * E1), sqrt(1.0 - e) * cos(0.5 * E1)) / DEGREE;
  *r_hat = (1.0 - e * cos(E1));

  return 0;
}

/**
 * Calculates a rectangular equatorial position and velocity vector for the given orbital elements
 * for the specified time of observation, in the native coordinate system in which the orbital is
 * defined (e.g. ecliptic for heliocentric orbitals).
 *
 * REFERENCES:
 * <ol>
 * <li>E.M. Standish and J.G. Williams 1992.</li>
 * <li>https://ssd.jpl.nasa.gov/planets/approx_pos.html</li>
 * <li>https://en.wikipedia.org/wiki/Orbital_elements</li>
 * <li>https://orbitalofficial.com/</li>
 * <li>https://downloads.rene-schwarz.com/download/M001-Keplerian_Orbit_Elements_to_Cartesian_State_Vectors.pdf</li>
 * </ol>
 *
 * @param jd_tdb    [day] Barycentric Dynamic Time (TDB) based Julian date
 * @param orbit     Orbital parameters
 * @param[out] pos  [AU] Output ICRS equatorial position vector around orbital center, or NULL if
 *                  not required.
 * @param[out] vel  [AU/day] Output ICRS velocity vector rel. to orbital center, in the native
 *                  system of the orbital, or NULL if not required. 0 if successful, or else -1 if
 *                  the orbital parameters are NULL, or if the position and velocity output
 *                  vectors are the same or the orbital system is ill defined (errno set to
 *                  EINVAL), or if the calculation did not converge (errno set to ECANCELED).
 *
 * @author Attila Kovacs
 * @since 1.4
 *
 * @sa novas_geom_posvel(), ephemeris(), make_orbital_object()
 */
int novas_orbit_native_posvel(double jd_tdb, const novas_orbital *restrict orbit, double *restrict pos, double *restrict vel) {
  static const char *fn = "novas_orbit_native_posvel";

  double dt, E = 0.0, nu = 0.0, r_hat = 0.0, omega, Omega;
  double cO, sO, ci, si, co, so;
  double xx, yx, zx, xy, yy, zy;

  if(!orbit)
    return novas_error(-1, EINVAL, fn, "input orbital elements is NULL");

  if(pos == vel)
    return novas_error(-1, EINVAL, fn, "output pos = vel (@ %p)", pos);

  dt = (jd_tdb - orbit->jd_tdb);

  prop_error(fn, novas_orbital_plane_pos(orbit->M0 + orbit->n * dt, orbit->e, &E, &r_hat, &nu), 0)

  E *= DEGREE;
  nu *= DEGREE;

  omega = orbit->omega * DEGREE;
  if(orbit->apsis_period > 0.0)
    omega += TWOPI * remainder(dt / orbit->apsis_period, 1.0);

  Omega = orbit->Omega * DEGREE;
  if(orbit->node_period > 0.0)
    Omega += TWOPI * remainder(dt / orbit->node_period, 1.0);

  // pos = Rz(-Omega) . Rx(-i) . Rz(-omega) . orb
  cO = cos(Omega);
  sO = sin(Omega);
  ci = cos(orbit->i * DEGREE);
  si = sin(orbit->i * DEGREE);
  co = cos(omega);
  so = sin(omega);

  // Rotation matrix, see E.M. Standish and J.G. Williams 1992.
  xx = cO * co - sO * so * ci;
  yx = sO * co + cO * so * ci;
  zx = si * so;

  xy = -cO * so - sO * co * ci;
  yy = -sO * so + cO * co * ci;
  zy = si * co;

  if(pos) {
    double x = orbit->a * r_hat * cos(nu);
    double y = orbit->a * r_hat * sin(nu);

    // Perform rotation
    pos[0] = xx * x + xy * y;
    pos[1] = yx * x + yy * y;
    pos[2] = zx * x + zy * y;
  }

  if(vel) {
    double v = orbit->n * DEGREE * orbit->a / r_hat;    // [AU/day]
    double x = -v * sin(E);
    double y = v * sqrt(1.0 - orbit->e * orbit->e) * cos(E);

    // Perform rotation
    vel[0] = xx * x + xy * y;
    vel[1] = yx * x + yy * y;
    vel[2] = zx * x + zy * y;
  }

  return 0;
}

/**
 * Calculates a rectangular equatorial position and velocity vector for the given orbital elements
 * for the specified time of observation.
 *
 * REFERENCES:
 * <ol>
 * <li>E.M. Standish and J.G. Williams 1992.</li>
 * <li>https://ssd.jpl.nasa.gov/planets/approx_pos.html</li>
 * <li>https://en.wikipedia.org/wiki/Orbital_elements</li>
 * <li>https://orbitalofficial.com/</li>
 * <li>https://downloads.rene-schwarz.com/download/M001-Keplerian_Orbit_Elements_to_Cartesian_State_Vectors.pdf</li>
 * </ol>
 *
 * @param jd_tdb    [day] Barycentric Dynamic Time (TDB) based Julian date
 * @param orbit     Orbital parameters
 * @param accuracy  NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1).
 * @param[out] pos  [AU] Output ICRS equatorial position vector around orbital center, or NULL if
 *                  not required.
 * @param[out] vel  [AU/day] Output ICRS equatorial velocity vector rel. to orbital center, or
 *                  NULL if not required.
 * @return          0 if successful, or else -1 if the orbital parameters are NULL,
 *                  or if the position and velocity output vectors are the same or the orbital
 *                  system is ill defined (errno set to EINVAL), or if the calculation did not
 *                  converge (errno set to ECANCELED).
 *
 * @author Attila Kovacs
 * @since 1.2
 *
 * @sa novas_geom_posvel(), ephemeris(), make_orbital_object()
 */
int novas_orbit_posvel(double jd_tdb, const novas_orbital *restrict orbit, enum novas_accuracy accuracy,
        double *restrict pos, double *restrict vel) {
  static const char *fn = "novas_orbit_posvel";

  prop_error(fn, novas_orbit_native_posvel(jd_tdb, orbit, pos, vel), 0);

  if(pos)
    prop_error(fn, orbit2gcrs(jd_tdb, &orbit->system, accuracy, pos), 0);

  if(vel)
    prop_error(fn, orbit2gcrs(jd_tdb, &orbit->system, accuracy, vel), 0);

  return 0;
}

/**
 * Sets the orientation of an orbital system using the RA and DEC coordinates of the pole
 * of the Laplace (or else equatorial) plane relative to which the orbital elements are
 * defined. Orbital parameters of planetary satellites normally include the R.A. and
 * declination of the pole of the local Laplace plane in which the Keplerian orbital elements
 * are referenced.
 *
 * The system will become referenced to the equatorial plane, the relative obliquity is set
 * to (90&deg; - `dec`), while the argument of the ascending node ('Omega') is set to
 * (90&deg; + `ra`).
 *
 * NOTES:
 * <ol>
 * <li>You should not expect much precision from the long-range orbital approximations for
 * planetary satellites. For applications that require precision at any level, you should rely
 * on appropriate ephemerides, or else on up-to-date short-term orbital elements.</li>
 * </ol>
 *
 * @param type  Coordinate reference system in which `ra` and `dec` are defined (e.g. NOVAS_GCRS).
 * @param ra    [h] the R.A. of the pole of the oribtal reference plane.
 * @param dec   [deg] the declination of the pole of the oribtal reference plane.
 * @param[out]  sys   Orbital system
 * @return      0 if successful, or else -1 (errno will be set to EINVAL) if the output `sys`
 *              pointer is NULL.
 *
 * @author Attila Kovacs
 * @since 1.2
 *
 * @sa make_orbital_object()
 */
int novas_set_orbsys_pole(enum novas_reference_system type, double ra, double dec, novas_orbital_system *restrict sys) {
  static const char *fn = "novas_set_orbsys_pole";

  if(type < 0 || type >= NOVAS_TIRS)
    return novas_error(-1, EINVAL, fn, "invalid orbital system type: %d", type);

  if(!sys)
    return novas_error(-1, EINVAL, fn, "input system is NULL");

  sys->plane = NOVAS_EQUATORIAL_PLANE;
  sys->type = type;
  sys->obl = remainder(90.0 - dec, DEG360);
  sys->Omega = remainder(15.0 * ra + 90.0, DEG360);

  return 0;
}



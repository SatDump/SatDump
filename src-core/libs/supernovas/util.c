/**
 * @file
 *
 * @date Created  on Mar 6, 2025
 * @author G. Kaplan and A. Kovacs
 *
 *  Various commonly used routines used throughout the SuperNOVAS library.
 */

#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/// \cond PRIVATE
#define __NOVAS_INTERNAL_API__    ///< Use definitions meant for internal use by SuperNOVAS only
#include "novas.h"
/// \endcond

/// Current debugging state for reporting errors and traces to stderr.
static enum novas_debug_mode novas_debug_state = NOVAS_DEBUG_OFF;

/**
 * Maximum number of iterations for convergent inverse calculations. Most iterative inverse
 * functions should normally converge in a handful of iterations. In some pathological cases more
 * iterations may be required. This variable sets an absolute maximum for the number of iterations
 * in order to avoid runaway (zombie) behaviour. If inverse functions fail to converge, they will
 * return a value indicating an error, and errno should be set to ECANCELED.
 *
 * @since 1.1
 */
int novas_inv_max_iter = 100;

/// \cond PRIVATE

/**
 * (<i>for internal use</i>) Propagates an error (if any) with an offset. If the error is
 * non-zero, it returns with the offset error value. Otherwise it keeps going as if it weren't
 * even there...
 *
 * @param loc     Function [:location] where error was produced.
 * @param n       error code that was received.
 * @param offset  Offset to add to error code (if &lt;0) to return.
 *
 * @since 1.0
 * @author Attila Kovacs
 *
 * @sa novas_error()
 */
int novas_trace(const char *restrict loc, int n, int offset) {
  if(n != 0) {
    n = n < 0 ? -1 : n + offset;
    if(novas_get_debug_mode() != NOVAS_DEBUG_OFF)
      fprintf(stderr, "       @ %s [=> %d]\n", loc, n);
  }
  return n;
}

/**
 * (<i>for internal use</i>) Traces an error before returning NAN.
 *
 * @param loc     Function [:location] where error was produced.
 * @return       NAN
 *
 * @since 1.1.1
 * @author Attila Kovacs
 */
double novas_trace_nan(const char *restrict loc) {
  if(novas_get_debug_mode() != NOVAS_DEBUG_OFF) {
    fprintf(stderr, "       @ %s [=> NAN]\n", loc);
  }
  return NAN;
}

/**
 * (<i>for internal use</i>) Sets an errno and report errors to the standard error, depending
 * on the current debug mode.
 *
 * @param en    {int} UNIX error number (see errno.h)
 * @param from  {string} Function (:location) where error originated
 * @param desc  {string} Description of error, with information to convey to user.
 *
 * @since 1.0
 * @author Attila Kovacs
 *
 * @sa novas_error(), novas_debug()
 */
void novas_set_errno(int en, const char *restrict from, const char *restrict desc, ...) {
  va_list varg;

  va_start(varg, desc);
  if(novas_get_debug_mode() != NOVAS_DEBUG_OFF) {
    fprintf(stderr, "\n  ERROR! %s: ", from);
    vfprintf(stderr, desc, varg);
    fprintf(stderr, "\n");
  }
  va_end(varg);

  errno = en;
}

/**
 * (<i>for internal use</i>) Sets errno and reports errors to the standard error, depending
 * on the current debug mode, before returning the supplied return code.
 *
 * @param ret   return value
 * @param en    UNIX error code (see errno.h)
 * @param from  function (:location) where error originated
 * @param desc  description of error, with information to convey to user.
 *
 * @since 1.0
 * @author Attila Kovacs
 *
 * @sa novas_set_errno(), novas_trace()
 */
int novas_error(int ret, int en, const char *restrict from, const char *restrict desc, ...) {
  va_list varg;

  va_start(varg, desc);
  if(novas_get_debug_mode() != NOVAS_DEBUG_OFF) {
    fprintf(stderr, "\n  ERROR! %s: ", from);
    vfprintf(stderr, desc, varg);
    fprintf(stderr, " [=> %d]\n", ret);
  }
  va_end(varg);

  errno = en;
  return ret;
}

/**
 * (<i>for internal use only</i>) Calculates the distance-swuared between two 3-vectors.
 *
 * @param v1    Pointer to a 3-component (x, y, z) vector. The argument cannot be NULL
 * @param v2    Pointer to another 3-component (x, y, z) vector. The argument cannot
 *              be NULL
 * @return      The distance squared between the two vectors
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_vlen(), novas_vdot()
 */
double novas_vdist2(const double *v1, const double *v2) {
  double d2 = 0.0;
  int i;
  for(i = 3; --i >= 0;) {
    const double d = v1[i] - v2[i];
    d2 += d * d;
  }
  return d2;
}

/**
 * (<i>for internal use only</i>) Calculates the distance between two 3-vectors.
 *
 * @param v1    Pointer to a 3-component (x, y, z) vector. The argument cannot be NULL
 * @param v2    Pointer to another 3-component (x, y, z) vector. The argument cannot
 *              be NULL
 * @return      The distance between the two vectors
 *
 * @since 1.0
 * @author Attila Kovacs
 *
 * @sa novas_vlen(), novas_vdot()
 */
double novas_vdist(const double *v1, const double *v2) {
  return sqrt(novas_vdist2(v1, v2));
}

/**
 * (<i>for internal use only</i>) Calculates the dot product between two 3-vectors.
 *
 * @param v1    Pointer to a 3-component (x, y, z) vector. The argument cannot be NULL
 * @param v2    Pointer to another 3-component (x, y, z) vector. The argument cannot
 *              be NULL
 * @return      The dot product between the two vectors.
 *
 * @since 1.0
 * @author Attila Kovacs
 *
 * @sa novas_vlen(), novas_vdist()
 */
double novas_vdot(const double *v1, const double *v2) {
  return (v1[0] * v2[0]) + (v1[1] * v2[1]) + (v1[2] * v2[2]);
}

/**
 * (<i>for internal use only</i>) Checks if two Julian dates are equal with regard to the
 * dynamic equator. The two dates are considered equal if they agree within 10<sup>-7</sup>
 * days (or about 10 ms).
 *
 * For reference, woth a precession rate of ~50"/year, the precession in in 10 ms is
 * on the order of 0.015 uas, which is well below the promised sub-uas precision even in
 * for the highest accuracy calculations. As such it is safe to use the reduced accuracy time
 * check for cached precession-related quantities.
 *
 * @param accuracy  NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param jd1       [day] a Julian date (in any time measure)
 * @param jd2       [day] a Julian date in the same time measure as the first argument
 * @return          TRUE (1) if the two dates are effectively the same at the precision of
 *                  comparison, or else FALSE (0) if they differ by more than the allowed
 *                  tolerance.
 *
 * @since 1.0
 * @author Attila Kovacs
 */
int novas_time_equals(double jd1, double jd2) {
  return fabs(jd1 - jd2) < 1e-7;
}

/**
 * (<i>for internal use only</i>) Checks if two Julian dates are precisely equal with regard to
 * the dynamic equator. The two dates are considered equal if they agree within 10<sup>-7</sup>
 * days (or about 10 ms) of each other in reduced accuracy or if they agree within 10<sup>-9</sup>
 * days (or about 100 us), corresponding to double-precision limits, in full accuracy mode.
 *
 * For reference, woth a precession rate of ~50"/year, the precession in in 10 ms is
 * on the order of 0.015 uas, which is well below the promised sub-uas precision even in
 * for the highest accuracy calculations. As such it is safe to use the reduced accuracy time
 * check for cached precession-related quantities.
 *
 * @param accuracy  NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param jd1       [day] a Julian date (in any time measure)
 * @param jd2       [day] a Julian date in the same time measure as the first argument
 * @return          TRUE (1) if the two dates are effectively the same at the precision of
 *                  comparison, or else FALSE (0) if they differ by more than the allowed
 *                  tolerance.
 *
 * @since 1.0
 * @author Attila Kovacs
 */
int novas_time_equals_hp(double jd1, double jd2) {
  return fabs(jd1 - jd2) < 1e-9;
}

/**
 * Computationally efficient implementation of 3D rotation with small angles.
 *
 * @param in            3D vector to rotate
 * @param ax            (rad) rotation angle around x
 * @param ay            (rad) rotation angle around y
 * @param az            (rad) rotation angle around z
 * @param[out] out      Rotated vector. It can be the same as the input.
 *
 * @since 1.0
 * @author Attila Kovacs
 */
void novas_tiny_rotate(const double *in, double ax, double ay, double az, double *out) {
  const double x = in[0], y = in[1], z = in[2];
  const double A[3] = { ax * ax, ay * ay, az * az };

  out[0] = x - 0.5 * (A[1] + A[2]) * x - az * y + ay * z;
  out[1] = y - 0.5 * (A[0] + A[2]) * y + az * x - ax * z;
  out[2] = z - 0.5 * (A[0] + A[1]) * z - ay * x + ax * y;
}

/// \endcond

/// ==========================================================================

/**
 * Enables or disables reporting errors and traces to the standard error stream.
 *
 * @param mode    NOVAS_DEBUG_OFF (0; or &lt;0), NOVAS_DEBUG_ON (1), or NOVAS_DEBUG_EXTRA (2; or
 *                &gt;2).
 *
 * @since 1.0
 * @author Attila Kovacs
 *
 * @sa novas_get_debug_mode()
 */
void novas_debug(enum novas_debug_mode mode) {
  if(mode < 0 || mode > NOVAS_DEBUG_EXTRA)
    mode = NOVAS_DEBUG_EXTRA;

  novas_debug_state = mode;
}

/**
 * Returns the current, thread-local, mode for reporting errors encountered (and traces).
 *
 * @return    The current debug mode in the calling thread.
 *
 * @since 1.0
 * @author Attila Kovacs
 *
 * @sa novas_debug()
 */
enum novas_debug_mode novas_get_debug_mode() {
  return novas_debug_state;
}

/**
 * Returns the normalized angle in the [0:2&pi;) range.
 *
 * @param angle   [rad] an angle in radians.
 * @return        [rad] the normalized angle in the [0:2&pi;) range.
 *
 * @since 1.0
 * @author Attila Kovacs
 */
double novas_norm_ang(double angle) {
  double a;

  a = remainder(angle, TWOPI);
  if(a < 0.0)
    a += TWOPI;

  return a;
}

/**
 * (<i>for internal use only</i>) Calculates the length of a 3-vector
 *
 * @param v     Pointer to a 3-component (x, y, z) vector. The argument cannot be NULL
 * @return      the length of the vector
 *
 * @since 1.0
 * @author Attila Kovacs
 *
 * @sa novas_vdot(), novas_vdist()
 */
double novas_vlen(const double *restrict v) {
  return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

/**
 * Transforms a vector from one coordinate system to another with same origin and axes rotated
 * about the z-axis.
 *
 * REFERENCES:
 * <ol>
 * <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 * </ol>
 *
 * @param angle   [deg] Angle of coordinate system rotation, positive counterclockwise when
 *                viewed from +z, in degrees.
 * @param in      Input position vector.
 * @param[out] out    Position vector expressed in new coordinate system rotated about z by
 *                    'angle'. It can be the same vector as the input.
 *
 * @return        0 if successful, or -1 if the output vector is NULL.
 *
 */
int spin(double angle, const double *in, double *out) {
  static THREAD_LOCAL double ang_last = -999.0;
  static THREAD_LOCAL double xx, yx, xy, yy, zz = 1.0;

  double x, y, z;

  if(!in || !out)
    return novas_error(-1, EINVAL, "spin", "NULL input or output 3-vector: in=%p, out=%p", in, out);

  angle = remainder(angle * DEGREE, TWOPI);

  if(fabs(angle - ang_last) >= 1.0e-12) {
    const double cosang = cos(angle);
    const double sinang = sin(angle);

    // Rotation matrix (non-zero elements only).
    xx = cosang;
    yx = sinang;
    xy = -sinang;
    yy = cosang;

    ang_last = angle;
  }

  x = in[0];
  y = in[1];
  z = in[2];

  // Perform rotation.
  out[0] = xx * x + yx * y;
  out[1] = xy * x + yy * y;
  out[2] = zz * z;

  return 0;
}

/**
 * Converts an vector in equatorial rectangular coordinates to equatorial spherical
 * coordinates.
 *
 * REFERENCES:
 * <ol>
 * <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 * </ol>
 *
 * @param pos       Position 3-vector, equatorial rectangular coordinates.
 * @param[out] ra   [h] Right ascension in hours [0:24] or NAN if the position vector is NULL or a
 *                  null-vector. It may be NULL if notrequired.
 * @param[out] dec  [deg] Declination in degrees [-90:90] or NAN if the position vector is NULL or
 *                  a null-vector. It may be NULL if not required.
 * @return          0 if successful, -1 of any of the arguments are NULL, or
 *                  1 if all input components are 0 so 'ra' and 'dec' are indeterminate, or else 2
 *                  if both x and y are zero, but z is nonzero, and so 'ra' is indeterminate.
 *
 * @sa radec2vector()
 */
short vector2radec(const double *restrict pos, double *restrict ra, double *restrict dec) {
  static const char *fn = "vector2radec";
  double xyproj;

  // Default return values.
  if(ra)
    *ra = NAN;
  if(dec)
    *dec = NAN;

  if(!pos)
    return novas_error(-1, EINVAL, fn, "NULL input or output pointer: pos=%p, ra=%p, dec=%p", pos, ra, dec);

  xyproj = sqrt(pos[0] * pos[0] + pos[1] * pos[1]);
  if(xyproj == 0.0) {
    if(pos[2] == 0)
      return novas_error(1, EINVAL, fn, "all input components are zero");

    if(ra)
      *ra = 0.0;
    if(dec)
      *dec = (pos[2] < 0.0) ? -90.0 : 90.0;

    return novas_error(2, EDOM, fn, "indeterminate RA for equatorial pole input");
  }

  if(ra) {
    *ra = atan2(pos[1], pos[0]) / HOURANGLE;
    if(*ra < 0.0)
      *ra += DAY_HOURS;
  }
  if(dec)
    *dec = atan2(pos[2], xyproj) / DEGREE;

  return 0;
}

/**
 * Converts equatorial spherical coordinates to a vector (equatorial rectangular coordinates).
 *
 * @param ra          [h] Right ascension (hours).
 * @param dec         [deg] Declination (degrees).
 * @param dist        [AU] Distance (AU)
 * @param[out] pos    [AU] Position 3-vector, equatorial rectangular coordinates (AU).
 *
 * @return            0 if successful, or -1 if the vector argument is NULL.
 *
 * @sa vector2radec(), starvectors()
 */
int radec2vector(double ra, double dec, double dist, double *restrict pos) {
  double cosdec;

  if(!pos)
    return novas_error(-1, EINVAL, "radec2vector", "NULL output 3-vector");

  dec *= DEGREE;
  ra *= HOURANGLE;

  cosdec = cos(dec);

  pos[0] = dist * cosdec * cos(ra);
  pos[1] = dist * cosdec * sin(ra);
  pos[2] = dist * sin(dec);

  return 0;
}

/**
 * Returns the difference in light-time, for a star, between the barycenter of the solar system
 * and the observer (or the geocenter) (<i>Usage A</i>).
 *
 * Alternatively (<i>Usage B</i>), this function returns the light-time from the observer (or the
 * geocenter) to a point on a light ray that is closest to a specific solar system body.  For this
 * purpose, 'pos_src' is the position vector toward observed object, with respect to origin at
 * observer (or the geocenter); 'pos_body' is the position vector of solar system body, with
 * respect to origin at observer (or the geocenter), components in AU; and the returned value is
 * the light time to point on line defined by 'pos' that is closest to solar system body (positive
 * if light passes body before hitting observer, i.e., if 'pos_body' is within 90 degrees of
 * 'pos_src').
 *
 * NOTES:
 * <ol>
 * <li>This function is called by `novas_geom_posvel()`, novas_sky_pos(), or `place()`</li>
 * </ol>
 *
 * @param pos_src   Position vector towards observed object, with respect to the SSB
 *                  (<i>Usage A</i>), or relative to the observer / geocenter (<i>Usage B</i>).
 * @param pos_body  [AU] Position of observer relative to SSB (<i>Usage A</i>), or position of
 *                  intermediate solar-system body with respect to the observer / geocenter
 *                  (<i>Usage B</i>).
 * @return          [day] Difference in light time to observer, either relative to SSB
 *                  (<i>Usage A</i>) or relative intermediate solar-system body
 *                  (<i>Usage B</i>); or else NAN if either of the input arguments is NULL.
 *
 * @sa novas_sky_pos(), novas_geom_posvel()
 */
double d_light(const double *pos_src, const double *pos_body) {
  double d_src;

  if(!pos_src || !pos_body) {
    novas_set_errno(EINVAL, "d_light", "NULL input 3-vector: pos_src=%p, pos_body=%p [=> NAN]", pos_src, pos_body);
    return NAN;
  }

  d_src = novas_vlen(pos_src);

  // Light-time returned is the projection of vector 'pos_obs' onto the
  // unit vector 'u1' (formed from 'pos_body'), divided by the speed of light.
  return d_src > 1e-30 ? novas_vdot(pos_body, pos_src) / d_src / C_AUDAY : 0.0;
}

/**
 * Returns the angular separation of two locations on a sphere.
 *
 * @param lon1    [deg] longitude of first location
 * @param lat1    [deg] latitude of first location
 * @param lon2    [deg] longitude of second location
 * @param lat2    [deg] latitude of second location
 * @return        [deg] the angular separation of the two locations.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_equ_sep(), novas_sun_angle(), novas_moon_angle()
 */
double novas_sep(double lon1, double lat1, double lon2, double lat2) {
  double c = sin(lat1 * DEGREE) * sin(lat2 * DEGREE) + cos(lat1 * DEGREE) * cos(lat2 * DEGREE) * cos((lon1 - lon2) * DEGREE);
  double c2 = c * c;
  // Ensure that argument to sqrt() is not negative given rounding errors.
  return atan2(c2 < 1.0 ? sqrt(1.0 - c2) : 0.0, c) / DEGREE;
}

/**
 * Returns the angular separation of two equatorial locations on a sphere.
 *
 * @param ra1     [h] right ascension of first location
 * @param dec1    [deg] declination of first location
 * @param ra2     [h] right ascension of second location
 * @param dec2    [deg] declination of second location
 * @return        [deg] the angular separation of the two locations.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_sep(), novas_sun_angle(), novas_moon_angle()
 */
double novas_equ_sep(double ra1, double dec1, double ra2, double dec2) {
  return novas_sep(15.0 * ra1, dec1, 15.0 * ra2, dec2);
}

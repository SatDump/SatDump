/**
 * @file
 *
 * @date Created  on Aug 26, 2025
 * @author Attila Kovacs
 *
 *  Transformations of station coordinates, velocities, and Earth orinetation parameters (EOP)
 *  between various ITRF realizations, and conversion between Cartesian (x, y, z) and geodetic
 *  (longitude, latitude, altitude) coordinates w.r.t. the reference ellipsoid.
 *
 *  REFERENCES:
 *  <ol>
 *  <li>ITRS Conventions Chapter 4, see https://iers-conventions.obspm.fr/content/chapter4/icc4.pdf</li>
 *  </ol>
 */

/// \cond PRIVATE
#define __NOVAS_INTERNAL_API__      ///< Use definitions meant for internal use by SuperNOVAS only
/// \endcond

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "novas.h"

/// \cond PRIVATE
typedef struct {
  double T[3];      ///< [mm]   Translation components
  double D;         ///< [ppb]  scaling
  double R[3];      ///< [mas]  (small) rotation angles
} itrf_terms;

typedef struct {
  int year;		        ///< [yr] Realization yesr, e.g. 1992 for ITRF92, etc.
  double epoch;       ///< [yr] Epoch for which values are defined
  itrf_terms terms;	  ///< The time invariant transformation terms
  itrf_terms rates;	  ///< [/yr] The 1st order time dependence terms
} itrf_transform;

/**
 * Extracted from https://itrf.ign.fr/en/solutions/transformations/
 *
 * See also IERS Conventions, Chapter 5, Table 4.1, and from
 * https://itrf.ign.fr/en/solutions/itrf2020/
 *
 */
static const itrf_transform realizations[] = { //
        { 1988, 2015.0, //
                { {  24.5,   -3.9, -169.9 },  11.47, {    0.10,    0.00,    0.36 } }, //
                { {   0.1,   -0.6,   -3.1 },   0.12, {    0.00,    0.00,    0.02 } }  //
        }, //
        { 1989, 2015.0, //
                { {  29.5,   32.1, -145.9 },   8.37, {    0.00,    0.00,    0.36 } }, //
                { {   0.1,   -0.6,   -3.1 },   0.12, {    0.00,    0.00,    0.02 } }  //
        }, //
        { 1990, 2015.0, //
                { {  24.5,    8.1, -107.9 },   4.97, {    0.00,    0.00,    0.36 } }, //
                { {   0.1,   -0.6,   -3.1 },   0.12, {    0.00,    0.00,    0.02 } }  //
        }, //
        { 1991, 2015.0, //
                { {  26.5,   12.1,  -91.9 },   4.67, {    0.00,    0.00,    0.36 } }, //
                { {   0.1,   -0.6,   -3.1 },   0.12, {    0.00,    0.00,    0.02 } }  //
        }, //
        { 1992, 2015.0, //
                { {  14.5,   -1.9,  -85.9 },   3.27, {    0.00,    0.00,    0.36 } }, //
                { {   0.1,   -0.6,   -3.1 },   0.12, {    0.00,    0.00,    0.02 } }  //
        }, //
        { 1993, 2015.0, //
                { {  -65.8,   1.9,  -71.3 },   4.47, {   -3.36,   -4.33,    0.75 } }, //
                { {   -2.8,  -0.2,   -2.3 },   0.12, {   -0.11,   -0.19,    0.07 } }  //
        }, //
        { 1994, 2015.0, //
                { {    6.5,  -3.9,  -77.9 },   3.98, {    0.00,    0.00,    0.36 } }, //
                { {    0.1,  -0.6,   -3.1 },   0.12, {    0.00,    0.00,    0.02 } }  //
        }, //
        { 1996, 2015.0, //
                { {    6.5,  -3.9,  -77.9 },   3.98, {    0.00,    0.00,    0.36 } }, //
                { {    0.1,  -0.6,   -3.1 },   0.12, {    0.00,    0.00,    0.02 } }  //
        }, //
        { 1997, 2015.0, //
                { {    6.5,  -3.9,  -77.9 },   3.98, {    0.00,    0.00,    0.36 } }, //
                { {    0.1,  -0.6,   -3.1 },   0.12, {    0.00,    0.00,    0.02 } }  //
        }, //
        { 2000, 2015.0, //
                { {   -0.2,   0.8,  -34.2 },   2.25, {    0.00,    0.00,    0.00 } }, //
                { {    0.1,   0.0,   -1.7 },   0.11, {    0.00,    0.00,    0.00 } }  //
        }, //
        { 2005, 2015.0, //
                { {    2.7,   0.1,   -1.4 },   0.65, {    0.00,    0.00,    0.00 } }, //
                { {    0.3,  -0.1,    0.1 },   0.03, {    0.00,    0.00,    0.00 } }  //
        }, //
        { 2008, 2015.0, //
                { {    0.2,   1.0,    3.3 },  -0.29, {    0.00,    0.00,    0.00 } }, //
                { {    0.2,   1.0,    3.3 },  -0.29, {    0.00,    0.00,    0.00 } }  //
        }, //
        { 2014, 2015.0,
                { {   -1.4,  -0.9,    1.4 },  -0.42, {    0.00,    0.00,    0.00 } }, //
                { {    0.0,   0.1,    0.2 },   0.00, {    0.00,    0.00,    0.00 } }  //
        }, //
        { 2020, 2015.0, //
                {},
                {}
        }, //
        {}
};


/// \endcond

/**
 * Returns the matching ITRF realization year for the given input year. For inputs &lt; 1988, 1988
 * is returned (corresponding to the ITRF88 realization). Otherwise, it returns the realization
 * year, which matches, or else immediately precedes, the input year.
 *
 * @param y   [yr] Calendar year for which to match an ITRF realization year
 * @return    [yr] The best match ITRF realization year
 */
static const itrf_transform *get_transform(int y) {
  int i;

  for(i = 0; realizations[i].year; i++) {
    const itrf_transform *T = &realizations[i];

    if(y <= T->year)
      return T;
  }

  return &realizations[i - 1];
}

/**
 * Return the differential terms, in S.I. units (m, u, rad).
 *
 * @param a           A set of ITRF transformation terms.
 * @param b           Another set of ITRF transformation terms.
 * @param[out] diff   Difference terms to populate with terms in standard units (m, u, rad).
 */
static void get_diff_terms(const itrf_terms *a, const itrf_terms *b, itrf_terms *diff) {
  int i;

  diff->D = (a->D - b->D) * 1e-9;             /// [ppb] -> [u]

  for(i = 3; --i >= 0; ) {
    diff->T[i] = (a->T[i] - b->T[i]) * 1e-3;  /// [mm] -> [m]
    diff->R[i] = (a->R[i] - b->R[i]) * MAS;   /// [mas] -> [rad]
  }
}

/**
 * Calculates the rotational offset of a vector due to small changes of orientation of the
 * ITRF frame.
 *
 * REFERENCES:
 * <ol>
 * <li>IERS Conventions, Chapter 4, Eq. 4.3, see https://iers-conventions.obspm.fr/content/chapter4/icc4.pdf</li>
 * </ol>
 *
 * @param[in] in      Input 3-vector
 * @param R           [rad] Small angle rotation angles R1, R2, R3
 * @param[out] out    Output 3-vector populated with the rotational offsets.
 */
static void itrf_rotation(const double *in, const double *R, double *out) {
  out[0] = -R[2] * in[1] + R[2] * in[2];
  out[1] = R[2] * in[0] - R[0] * in[2];
  out[2] = -R[1] * in[0] + R[0] * in[1];
}

/**
 * Converts ITRF coordinates between different realizations of the ITRF coordinate system. It
 * does not account for station motions, which the user should apply separately. For example,
 * consider the use case when input coordinates are given in ITRF88, for measurement in the epoch
 * 1994.36, and output is expected in ITRF2000 for measurements at 2006.78. This function simply
 * translates the input, measured in epoch 1994.36, to ITRF2000. Proper motion between the epochs
 * (2006.78 and 1994.36) can be calculated with the input rates before conversion, e.g.:
 *
 * ```c
 *   // Apply station motion in ITRF88
 *   for(i = 0; i < 3; i++)
 *     from_coords[i] += from_rates[i] * (2006.78 - 1994.36)
 *
 *   // Convert ITRF88 coordinates to ITRF2000
 *   novas_itrf_transform(1988, from_coords, from_rates, 2000, ...);
 * ```
 *
 * or equivalently, after the transformation to ITRF2000, as:
 *
 * ```c
 *   // Convert ITRF88 coordinates to ITRF2000
 *   novas_itrf_transform(1988, from_coords, from_rates, 2000, to_coords, to_rates);
 *
 *   // Apply station motion in ITRF2000
 *   for(i = 0; i < 3; i++)
 *     to_coords[i] += to_rates[i] * (2006.78 - 1994.36)
 * ```
 *
 * REFERENCES:
 * <ol>
 * <li>IERS Conventions, Chapter 4, Eq. 4.13 and Table 4.1. See
 * https://iers-conventions.obspm.fr/content/chapter4/icc4.pdf</li>
 * </ol>
 *
 * @param from_year       [yr] ITRF realization year of input coordinates / rates. E.g. 1992 for
 *                        ITRF92.
 * @param[in] from_coords [m] input ITRF coordinates.
 * @param[in] from_rates  [m/yr] input ITRF coordinate rates, or NULL if not known or needed.
 * @param to_year         [yr] ITRF realization year of output coordinates / rates. E.g. 2014 for
 *                        ITRF2014.
 * @param[out] to_coords  [m] ITRF coordinates at final year, or NULL if not required. It may be
 *                        the same as either of the inputs.
 * @param[out] to_rates   [m/yr] ITRF coordinate rates at final year, or NULL if not known or
 *                        needed. It may be the same as either of the inputs.
 * @return                0 if successful, or else -1 (errno set to EINVAL) if the input
 *                        coordinates are NULL, or if input rates are NULL _but_ output_rates are
 *                        not NULL.
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_itrf_transform_site(), novas_itrf_transform_eop()
 * @sa novas_geodetic_to_cartesian()
 */
int novas_itrf_transform(int from_year, const double *restrict from_coords, const double *restrict from_rates,
        int to_year, double *to_coords, double *to_rates) {
  static const char *fn = "novas_itrf_transform";

  const itrf_transform *from = get_transform((int) floor(from_year));
  const itrf_transform *to = get_transform((int) floor(to_year));
  const double dt = (to->epoch - from->epoch);

  itrf_terms T0 = {}, T1 = {};
  double x[3] = {0.0}, r[3] = {0.0}, RX[3], R1X[3];
  int i;

  if(!from_coords)
    return novas_error(-1, EINVAL, fn, "input coordinates are NULL");

  if(to_rates && !from_rates)
    return novas_error(-1, EINVAL, fn, "expecting output rates, but input rates are NULL");

  get_diff_terms(&to->terms, &from->terms, &T0);
  get_diff_terms(&to->rates, &from->rates, &T1);

  memcpy(x, from_coords, sizeof(x));
  itrf_rotation(x, T0.R, RX);
  itrf_rotation(x, T1.R, R1X);

  if(from_rates)
    memcpy(r, from_rates,  sizeof(r));

  for (i = 0; i < 3; i++) {
    // x' = x' + T' + D'x + R'(x)
    if(from_rates)
      r[i] += T1.T[i] + T1.D * x[i] + R1X[i];

    // x = x + (t-t0) x' + T + Dx + R(x) + (t - tk) * (T' + D'x + R'(x))
    if(to_coords)
      x[i] += T0.T[i] + T0.D * x[i] + RX[i] + dt * (T1.T[i] + T1.D * x[i] + R1X[i]);
  }

  if(to_coords)
    memcpy(to_coords, x, sizeof(x));

  if(to_rates)
    memcpy(to_rates, r, sizeof(r));

  return 0;
}

/**
 * Transforms Earth orientation parameters (xp, yp, dUT1) from one ITRF realization to another.
 * For the highest precision applications, observing sites should be defined in the same ITRF
 * realization as the IERS Earth orientation parameters (EOP). To reconcile you may transform
 * either the site location or the EOP between different realizations to match.
 *
 * REFERENCES:
 * <ol>
 * <li>IERS Conventions, Chapter 4, Eq. 4.14 and Table 4.1. See
 * https://iers-conventions.obspm.fr/content/chapter4/icc4.pdf</li>
 * </ol>
 *
 *
 * @param from_year       [yr] ITRF realization year of input coordinates / rates. E.g. 1992 for
 *                        ITRF92.
 * @param[in] from_xp     [arcsec] x-pole Earth orientation parameter (angle) in the input ITRF
 *                        realization.
 * @param[in] from_yp     [arcsec] y-pole Earth orientation parameter (angle) in the input ITRF
 *                        realization.
 * @param[in] from_dut1   [s] UT1-UTC time difference in the input ITRF realization.
 * @param to_year         [yr] ITRF realization year of input coordinates / rates. E.g. 2000 for
 *                        ITRF2000.
 * @param[out] to_xp      [arcsec] x-pole Earth orientation parameter (angle) in the output ITRF
 *                        realization, or NULL if not required.
 * @param[out] to_yp      [arcsec] y-pole Earth orientation parameter (angle) in the output ITRF
 *                        realization, or NULL if not required.
 * @param[out] to_dut1    [s] UT1-UTC time difference in the output ITRF realization, or NULL if
 *                        not required.
 * @return                0
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_itrf_transform(), novas_itrf_transform_site()
 * @sa novas_make_frame(), novas_timespec, wobble()
 */
int novas_itrf_transform_eop(int from_year, double from_xp, double from_yp, double from_dut1,
        int to_year, double *restrict to_xp, double *restrict to_yp, double *restrict to_dut1) {
  const itrf_transform *from = get_transform((int) floor(from_year));
  const itrf_transform *to = get_transform((int) floor(to_year));

  itrf_terms T0 = {};

  get_diff_terms(&to->terms, &from->terms, &T0);

  // xp += R2
  if(to_xp)
    *to_xp = from_xp + T0.R[1] / ARCSEC;

  // yp += R1
  if(to_yp)
    *to_yp = from_yp + T0.R[0] / ARCSEC;

  // UT +- 1/f R3
  if(to_dut1)
    *to_dut1 = from_dut1 + T0.R[2] / NOVAS_EARTH_FLATTENING * (NOVAS_DAY / TWOPI);

  return 0;
}

/**
 * Transforms a geodetic location between two International Terrestrial Reference Frame (ITRF)
 * realizations. ITRF realizations differ at the mm / &mu;as level. Thus for the highest accuracy
 * astrometry, from e.g. VLBI sites, it may be desirable to ensure that the site coordinates are
 * defined for the same ITRF realization, as the one in which Earth-orientation parameters (EOP)
 * are provided for `novas_make_frame()`, `novas_timespec`, or `wobble()`.
 *
 * @param from_year       [yr] ITRF realization year of input coordinates / rates. E.g. 1992 for
 *                        ITRF92.
 * @param[in] in          Input site, defined in the original ITRF realization.
 * @param to_year         [yr] ITRF realization year of input coordinates / rates. E.g. 2000 for
 *                        ITRF2000.
 * @param[out] out        Output site, calculated for the final ITRF realization. It may be the
 *                        same as the input.
 * @return                0 if successful, or else -1 if either site pointer is NULL (errno set
 *                        to EINVAL).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_itrf_transform_eop(), novas_itrf_transform(), make_itrf_site(), make_itrf_observer()
 * @sa novas_transform_ellipsoid()
 */
int novas_itrf_transform_site(int from_year, const on_surface *in, int to_year, on_surface *out) {
  static const char *fn = "novas_itrf_transform_site";

  double xyz[3] = {0.0};

  if(!in)
    return novas_error(-1, EINVAL, fn, "input site is NULL");

  if(!out)
    return novas_error(-1, EINVAL, fn, "output site is NULL");

  novas_geodetic_to_cartesian(in->longitude, in->latitude, in->height, NOVAS_GRS80_ELLIPSOID, xyz);
  novas_itrf_transform(from_year, xyz, NULL, to_year, xyz, NULL);
  novas_cartesian_to_geodetic(xyz, NOVAS_GRS80_ELLIPSOID, &out->longitude, &out->latitude, &out->height);

  out->temperature = in->temperature;
  out->pressure = in->pressure;
  out->humidity = in->humidity;

  return 0;
}

/**
 * Transforms a geodetic location from one reference ellipsoid to another. For example to transform
 * a GPS location (defined on the WGS84 ellipsoid) to an International Terrestrial Reference Frame
 * (ITRF) location (defined on the GRS80 ellipsoid), or vice versa.
 *
 * @param from_ellipsoid    Reference ellipsoid of the input coordinates.
 * @param[in] in            Input site, defined on the original reference ellipsoid.
 * @param to_ellipsoid      Reference ellipsoid for which to calculate output coordinates.
 * @param[out] out          Output site, calculated for the final reference ellipsoid. It may be
 *                          the same as the input.
 * @return                  0 if successful, or else -1 if either site pointer is NULL (errno set
 *                          to EINVAL).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_itrf_transform_site(), make_gps_site(), make_itrf_site()
 */
int novas_geodetic_transform_site(enum novas_reference_ellipsoid from_ellipsoid, const on_surface *in,
        enum novas_reference_ellipsoid to_ellipsoid, on_surface *out) {
  static const char *fn = "novas_transform_ellipsoid";

  double xyz[3] = {0.0};

  if(!in)
    return novas_error(-1, EINVAL, fn, "input site is NULL");

  if(!out)
    return novas_error(-1, EINVAL, fn, "output site is NULL");

  prop_error(fn, novas_geodetic_to_cartesian(in->longitude, in->latitude, in->height, from_ellipsoid, xyz), 0);
  prop_error(fn, novas_cartesian_to_geodetic(xyz, to_ellipsoid, &out->longitude, &out->latitude, &out->height), 0);

  out->temperature = in->temperature;
  out->pressure = in->pressure;
  out->humidity = in->humidity;

  return 0;
}

/**
 * Returns the equatorial radius and flattening for the given Earth reference ellipsoid model.
 *
 * REFERENCES:
 * <ol>
 * <li>https://en.wikipedia.org/wiki/Earth_ellipsoid</li>
 * </ol>
 *
 * @param ellipsoid   The reference ellipsoid model
 * @param[out] a      [m] Equatorial radius
 * @param[out] f      Flattening
 * @return            0 if successful, or else -1 (errno set to EINVAL) if the ellipsoid is
 *                    invalid.
 */
static int get_ellipsoid(enum novas_reference_ellipsoid ellipsoid, double *a, double *f) {

  switch(ellipsoid) {
    case NOVAS_GRS80_ELLIPSOID:
      *a = NOVAS_GRS80_RADIUS;
      *f = NOVAS_GRS80_FLATTENING;
      break;
    case NOVAS_WGS84_ELLIPSOID:
      *a = NOVAS_WGS84_RADIUS;
      *f = NOVAS_WGS84_FLATTENING;
      break;
    case NOVAS_IERS_1989_ELLIPSOID:
      *a = 6378136.0;
      *f = 298.257;
      break;
    case NOVAS_IERS_2003_ELLIPSOID:
      *a = 6378136.6;
      *f = 298.25642;
      break;
    default:
      return novas_error(-1, EINVAL, "get_ellipsoid", "invalid reference ellipsoid: %d", ellipsoid);
  }

  return 0;
}

/**
 * Converts geodetic site coordinates to geocentric Cartesian coordinates, using the specified
 * reference ellipsoid.
 *
 * @param[in] lon     [deg] Geodetic longitude
 * @param[in] lat     [deg] Geodetic latitude
 * @param[in] alt     [m] Geodetic altitude (i.e. above sea level).
 * @param ellipsoid   Reference ellipsoid to use. For ITRF use `NOVAS_GRS80_ELLIPSOID`, for GPS
 *                    related applications use `NOVAS_WGS84_ELLIPSOID`.
 * @param[out] xyz    [m] Corresponding geocentric Cartesian coordinates (x, y, z) 3-vector.
 * @return            0 if successful, or else -1 if the output vector is NULL (errno is set to
 *                    EINVAL).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_cartesian_to_geodetic()
 * @sa novas_itrf_transform_site(), novas_itrf_transform()
 */
int novas_geodetic_to_cartesian(double lon, double lat, double alt, enum novas_reference_ellipsoid ellipsoid, double *xyz) {
  static const char *fn = "novas_geodetic_to_cartesian";

  double a = NOVAS_GRS80_RADIUS, f = NOVAS_GRS80_FLATTENING;
  double e2, sinlat, coslat, N;

  if(!xyz)
    return novas_error(-1, EINVAL, fn, "output Cartesian vector is NULL");

  prop_error(fn, get_ellipsoid(ellipsoid, &a, &f), 0);

  e2 = (2.0 - f) * f;

  lon *= DEGREE;
  lat *= DEGREE;

  sinlat = sin(lat);
  coslat = cos(lat);

  N = a / sqrt(1 - e2 * sinlat * sinlat);

  xyz[0] = (N + alt) * coslat * cos(lon);
  xyz[1] = (N + alt) * coslat * sin(lon);
  xyz[2] = (N * (1 - e2) + alt) * sinlat;

  return 0;
}

/**
 * Converts geocentric Cartesian site coordinates to geodetic coordinates on the given reference
 * ellipsoid.
 *
 * NOTES:
 * <ol>
 * <li>Adapted from the IERS `GCONV2.F` source code, see
 * https://iers-conventions.obspm.fr/content/chapter4/software/GCONV2.F.
 * </li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 * <li>
 *     Fukushima, T., "Transformation from Cartesian to geodetic
 *     coordinates accelerated by Halley's method", J. Geodesy (2006),
 *     79(12): 689-693
 * </li>
 * <li>
 *     Petit, G. and Luzum, B. (eds.), IERS Conventions (2010),
 *     IERS Technical Note No. 36, BKG (2010)
 * </li>
 * </ol>
 *
 * @param[in] xyz     [m] Input geocentric Cartesian coordinates (x, y, z) 3-vector.
 * @param ellipsoid   Reference ellipsoid to use. For ITRF use `NOVAS_GRS80_ELLIPSOID`, for GPS
 *                    related applications use `NOVAS_WGS84_ELLIPSOID`.
 * @param[out] lon    [deg] Geodetic longitude. It may be NULL if not required.
 * @param[out] lat    [deg] Geodetic latitude. It may be NULL if not required.
 * @param[out] alt    [m] Geodetic altitude (i.e. above sea level). It may be NULL if not
 *                    required.
 * @return            0 if successful, or else -1 if the input vector is NULL (errno is set to
 *                    EINVAL).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_geodetic_to_cartesian()
 * @sa novas_itrf_transform(), novas_itrf_transform_site()
 */
int novas_cartesian_to_geodetic(const double *restrict xyz, enum novas_reference_ellipsoid ellipsoid, double *restrict lon,
        double *restrict lat, double *restrict alt) {
  static const char *fn = "novas_cartesian_to_geodetic";

  double a = NOVAS_GRS80_RADIUS, f = NOVAS_GRS80_FLATTENING;
  double e2, ep2, ep, aep, az, p2;
  double phi = 0.0, lambda = 0.0, h = 0.0;

  if(!xyz)
    return novas_error(-1, EINVAL, fn, "input Cartesian vector is NULL");

  prop_error(fn, get_ellipsoid(ellipsoid, &a, &f), 0);

  // Compute distance from polar axis squared
  e2 = (2.0 - f) * f;
  ep2 = 1.0 - e2;
  ep = sqrt(ep2);
  aep = a * ep;
  az = fabs(xyz[2]);
  p2 = xyz[0] * xyz[0] + xyz[1] * xyz[1];

  // Compute longitude lambda
  lambda = p2 ? atan2(xyz[1], xyz[0]) : 0.0;

  if(!p2) {
    // Special case: pole.
    phi = 0.5 * M_PI;
    h = az - aep;
  }
  else {
    const double e4t = 1.5 * e2 * e2;

    // Compute distance from polar axis
    const double p = sqrt(p2);

    // Normalize
    const double s0 = az / a;
    const double pn = p / a;
    const double zp = ep * s0;

    // Prepare Newton correction factors.
    const double c0  = ep * pn;
    const double c02 = c0 * c0;
    const double c03 = c02 * c0;
    const double s02 = s0 * s0;
    const double s03 = s02 * s0;
    const double a02 = c02 + s02;
    const double a0  = sqrt(a02);
    const double a03 = a02 * a0;
    const double d0  = zp * a03 + e2 * s03;
    const double f0  = pn * a03 - e2 * c03;

    // Prepare Halley correction factor
    const double b0 = e4t * s02 * c02 * pn * (a0 - ep);
    const double s1 = d0 * f0 - b0 * s0;
    const double cp = ep * (f0 * f0 - b0 * c0);

    const double s12 = s1 * s1;
    const double cp2 = cp * cp;

    // Evaluate latitude and height.
    phi = atan2(s1, cp);
    h = (p * cp + az * s1 - a * sqrt(ep2 * s12 + cp2)) / sqrt(s12 + cp2);
  }

  // Restore sign of latitude.
  if (xyz[2] < 0.0) phi = -phi;

  if(lon)
    *lon = lambda / DEGREE;

  if(lat)
    *lat = phi / DEGREE;

  if(alt)
    *alt = h;

  return 0;
}


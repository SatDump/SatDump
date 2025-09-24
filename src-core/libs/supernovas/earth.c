/**
 * @file
 *
 * @date Created  on Mar 6, 2025
 * @author G. Kaplan and Attila Kovacs
 *
 *  Various functions relating to Earth position and rotation.
 */

#include <string.h>
#include <errno.h>

/// \cond PRIVATE
#define __NOVAS_INTERNAL_API__    ///< Use definitions meant for internal use by SuperNOVAS only
#include "novas.h"

/**
 * Structure to contain terms for librational and tidal corrections to earth orientation.
 */
typedef struct {
  int8_t n[6];    ///< [rad] gamma (GMST + &pi;), followed by the Delaunay arguments
  float A1;       ///< [&mu;as / &mu;s] Sine coefficient for first parameter (xp or UT1)
  float B1;       ///< [&mu;as / &mu;s] Cosine coefficient for first parameter (xp or UT1)
  float A2;       ///< [&mu;as / &mu;s] Sine coefficient for second parameter (yp or LOD)
  float B2;       ///< [&mu;as / &mu;s] Cosine coefficient for second parameter (yp or LOD)
} novas_eop_terms;

#include "eop/libration.tab.h"
#include "eop/tidal.tab.h"

/// \endcond


/**
 * Computes the position and velocity vectors of a terrestrial observer with respect to the
 * center of the Earth, based on the GRS80 reference ellipsoid, used for the International
 * Terrestrial Reference Frame (ITRF) and its realizations.
 *
 * This function ignores polar motion, unless the observer's longitude and latitude have been
 * corrected for it, and variation in the length of day (angular velocity of earth).
 *
 * The true equator and equinox of date do not form an inertial system.  Therefore, with
 * respect to an inertial system, the very small velocity component (several meters/day) due
 * to the precession and nutation of the Earth's axis is not accounted for here.
 *
 * REFERENCES:
 * <ol>
 * <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 * </ol>
 *
 * @param location    Location of observer in Earth's rotating frame
 * @param lst         [h] Local apparent sidereal time at reference meridian in hours.
 * @param[out] pos    [AU]  Position vector of observer with respect to center of Earth,
 *                    equatorial rectangular coordinates, referred to true equator
 *                    and equinox of date, components in AU. If reference meridian is
 *                    Greenwich and 'lst' = 0, 'pos' is
 *                    effectively referred to equator and Greenwich. (It may be NULL if no
 *                    position data is required).
 * @param[out] vel    [AU/day] Velocity vector of observer with respect to center of Earth,
 *                    equatorial rectangular coordinates, referred to true equator
 *                    and equinox of date, components in AU/day. (It must be distinct from
 *                    the pos output vector, and may be NULL if no velocity data is required).
 *
 * @return            0 if successful, or -1 if location is NULL or if the pos and vel output
 *                    arguments are identical pointers.
 *
 * @sa geo_posvel()
 * @sa make_gps_site(), make_itrf_site(), make_xyz_site()
 */
int terra(const on_surface *restrict location, double lst, double *restrict pos, double *restrict vel) {
  static const char *fn = "terra";
  double df, df2, phi, sinphi, cosphi, c, s, ach, ash, stlocl, sinst, cosst;
  double ht_km;
  int j;

  if(!location)
    return novas_error(-1, EINVAL, fn, "NULL observer location pointer");

  if(pos == vel)
    return novas_error(-1, EINVAL, fn, "identical output pos and vel 3-vectors @ %p", pos);

  // Compute parameters relating to geodetic to geocentric conversion.
  df = 1.0 - EF;
  df2 = df * df;

  phi = location->latitude * DEGREE;
  sinphi = sin(phi);
  cosphi = cos(phi);
  c = 1.0 / sqrt(cosphi * cosphi + df2 * sinphi * sinphi);
  s = df2 * c;
  ht_km = location->height / NOVAS_KM;
  ach = ERAD * c / NOVAS_KM + ht_km;
  ash = ERAD / NOVAS_KM * s + ht_km;

  // Compute local sidereal time factors at the observer's longitude.
  stlocl = lst * HOURANGLE + location->longitude * DEGREE;
  sinst = sin(stlocl);
  cosst = cos(stlocl);

  // Compute position vector components in kilometers.
  if(pos) {
    pos[0] = ach * cosphi * cosst;
    pos[1] = ach * cosphi * sinst;
    pos[2] = ash * sinphi;
  }

  // Compute velocity vector components in kilometers/sec.
  if(vel) {
    vel[0] = -ANGVEL * ach * cosphi * sinst;
    vel[1] = ANGVEL * ach * cosphi * cosst;
    vel[2] = 0.0;
  }

  // Convert position and velocity components to AU and AU/DAY.
  for(j = 0; j < 3; j++) {
    if(pos)
      pos[j] /= AU_KM;
    if(vel)
      vel[j] /= AU_KM / DAY;
  }

  return 0;
}

/**
 * Returns the Greenwich Mean Sidereal Time (GMST) for a given UT1 date, using eq. 42 from
 * Capitaine et al. (2003).
 *
 * REFERENCES:
 * <ol>
 * <li>Capitaine, N. et al. (2003), Astronomy and Astrophysics 412, 567-586, eq. (42).</li>
 * <li>IERS Conventions 2010, Chapter 5, Eq. 5.32, see at
 * https://iers-conventions.obspm.fr/content/chapter5/icc5.pdf</li>
 * </ol>
 *
 * @param jd_ut1      [day] UT1-based Julian Date.
 * @param ut1_to_tt   [s] UT1 - UTC time difference.
 * @return            [h] The Greenwich Mean Sidereal Time (GMST) in the 0-24 range.
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_gast(), novas_time_gst()
 */
double novas_gmst(double jd_ut1, double ut1_to_tt) {
  double deg = era(jd_ut1, 0.0) + novas_gmst_prec(jd_ut1 + ut1_to_tt / DAY) / 3600.0;
  double gst = remainder(deg / 15.0, DAY_HOURS);
  return (gst < 0.0) ? gst + DAY_HOURS : gst;
}

/**
 * Returns the Greenwich Apparent Sidereal Time (GAST) for a given UT1 date.
 *
 * @param jd_ut1      [day] UT1-based Julian Date.
 * @param ut1_to_tt   [s] UT1 - UTC time difference.
 * @param accuracy    NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @return            [h] The Greenwich Apparent Sidereal Time (GAST) in the 0-24 range.
 *
 * REFERENCES:
 * <ol>
 * <li>Capitaine, N. et al. (2003), Astronomy and Astrophysics 412, 567-586, eq. (42).</li>
 * <li>Kaplan, G. (2005), US Naval Observatory Circular 179.</li>
 * <li>IERS Conventions 2010, Chapter 5, Eq. 5.30 and Table 5.2e, see at
 * https://iers-conventions.obspm.fr/content/chapter5/icc5.pdf and
 * https://iers-conventions.obspm.fr/content/chapter5/additional_info/tab5.2e.txt</li>
 * </ol>
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_gmst()
 */
double novas_gast(double jd_ut1, double ut1_to_tt, enum novas_accuracy accuracy) {
  double ee = 0.0, gst;

  if(e_tilt(jd_ut1 + ut1_to_tt / DAY, accuracy, NULL, NULL, &ee, NULL, NULL) != 0)
    return novas_trace_nan("novas_gast");

  gst = remainder(novas_gmst(jd_ut1, ut1_to_tt) + ee / 3600.0, DAY_HOURS);
  return (gst < 0.0) ? gst + DAY_HOURS : gst;
}

/**
 * @deprecated Use novas_gmst() or novas_gast() instead to get the same results simpler.
 *
 * Computes the Greenwich sidereal time, either mean or apparent, at the specified Julian date.
 * The Julian date can be broken into two parts if convenient, but for the highest precision,
 * set 'jd_high' to be the integral part of the Julian date, and set 'jd_low' to be the fractional
 * part.
 *
 * NOTES:
 * <ol>
 * <li>Contains fix for known <a href="https://aa.usno.navy.mil/software/novas_faq">sidereal time
 * units bug.</a></li>
 * <li>As of version 1.5.0, the function uses Eq 42 of Capitaine et al. (2003) exclusively. In prior
 * versions, including NOVAS C the same method was provided twice, with the only difference being
 * an unnecessary change of coordinate basis to GCRS. This latter, mor roundabout way of getting
 * the same answer has been eliminated.<.li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 * <li>Capitaine, N. et al. (2003), Astronomy and Astrophysics 412, 567-586, eq. (42).</li>
 * <li>Kaplan, G. (2005), US Naval Observatory Circular 179.</li>
 * </ol>
 *
 * @param jd_ut1_high [day] High-order part of UT1 Julian date.
 * @param jd_ut1_low  [day] Low-order part of UT1 Julian date. (You can leave it at zero if
 *                    'jd_high' specified the date with sufficient precision)
 * @param ut1_to_tt   [s] TT - UT1 Time difference in seconds
 * @param gst_type    NOVAS_MEAN_EQUINOX (0) or NOVAS_TRUE_EQUINOX (1), depending on whether
 *                    wanting mean or apparent GST, respectively.
 * @param erot        Unused as of 1.5.0.
 * @param accuracy    NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] gst    [h] Greenwich (mean or apparent) sidereal time, in hours [0:24]. (In case
 *                    the returned error code is &gt;1 the gst value will be set to NAN.)
 * @return            0 if successful, or -1 if the 'gst' argument is NULL, 1 if 'accuracy' is
 *
 * @sa novas_gmst(), novas_gast(), novas_time_gst()
 * @sa era(), get_ut1_to_tt()
 */
short sidereal_time(double jd_ut1_high, double jd_ut1_low, double ut1_to_tt, enum novas_equinox_type gst_type,
        enum novas_earth_rotation_measure erot, enum novas_accuracy accuracy, double *restrict gst) {
  static const char *fn = "sidereal_time";

  (void) erot; // unused

  if(!gst)
    return novas_error(-1, EINVAL, fn, "NULL 'gst' output pointer");

  // Default return value
  *gst = NAN;

  if(accuracy != NOVAS_FULL_ACCURACY && accuracy != NOVAS_REDUCED_ACCURACY)
    return novas_error(1, EINVAL, fn, "invalid accuracy: %d", accuracy);

  jd_ut1_high += jd_ut1_low;
  *gst = (gst_type == NOVAS_TRUE_EQUINOX) ? novas_gast(jd_ut1_high, ut1_to_tt, accuracy) : novas_gmst(jd_ut1_high, ut1_to_tt);

  return 0;
}


/**
 * Returns the value of the Earth Rotation Angle (&theta;) for a given UT1 Julian date.  The
 * expression used is taken from the note to IAU Resolution B1.8 of 2000. The input Julian date
 * can be split into an into high and low order parts (e.g. integer and fractional parts) for
 * improved accuracy, or else one of the components (e.g. the low part) can be set to zero if no
 * split is desired.
 *
 * The algorithm used here is equivalent to the canonical &theta; = 0.7790572732640 +
 * 1.00273781191135448 * t, where t is the time in days from J2000 (t = jd_high + jd_low -
 * JD_J2000), but it avoids many two-PI 'wraps' that decrease precision (adopted from SOFA Fortran
 * routine `iau_era00`).
 *
 * REFERENCES:
 * <ol>
 *  <li>IERS Conventions 2010, Chapter 5, Eq. 5.15</li>
 *  <li>IAU Resolution B1.8, adopted at the 2000 IAU General Assembly, Manchester, UK.</li>
 *  <li>Kaplan, G. (2005), US Naval Observatory Circular 179.</li>
 * </ol>
 *
 * @param jd_ut1_high   [day] High-order part of UT1 Julian date.
 * @param jd_ut1_low    [day] Low-order part of UT1 Julian date.
 * @return              [deg] The Earth Rotation Angle (theta) in degrees [0:360].
 *
 * @sa novas_gast(), novas_time_gst()
 * @sa cirs_to_itrs(), itrs_to_cirs()
 */
double era(double jd_ut1_high, double jd_ut1_low) {
  double theta, thet1, thet2, thet3;

  thet1 = remainder(0.7790572732640 + 0.00273781191135448 * (jd_ut1_high - JD_J2000), 1.0);
  thet2 = remainder(0.00273781191135448 * jd_ut1_low, 1.0);
  thet3 = remainder(jd_ut1_high, 1.0) + remainder(jd_ut1_low, 1.0);

  theta = remainder(thet1 + thet2 + thet3, 1.0) * DEG360;
  if(theta < 0.0)
    theta += DEG360;

  return theta;
}

static void sum_eop_terms(const novas_eop_terms *terms, int n, double gmst, const novas_delaunay_args *da, double *a, double *b) {
  int i;
  double x[6];

  x[0] = remainder(gmst + 12.0, 24.0) * HOURANGLE;
  memcpy(&x[1], da, sizeof(novas_delaunay_args));

  if(a)
    *a = 0.0;
  if(b)
    *b = 0.0;

  for(i = 0; i < n; i++) {
    const novas_eop_terms *T = &terms[i];
    double arg = 0.0, c, s;
    int k;

    for(k = 0; k < 6; k++)
      arg += T->n[k] * x[k];

    s = sin(arg);
    c = cos(arg);

    if(a)
      *a += T->A1 * s + T->B1 * c;

    if(b)
      *b += T->A2 * s + T->B2 * c;
  }

  if(a)
    *a *= 1e-6; // [uas] -> [arcsec] / [us] -> [s]

  if(b)
    *b *= 1e-6; // [uas] -> [arcsec] / [us] -> [s]
}

/**
 * Calculate diurnal and semi-diurnal libration corrections to the Earth orientation parameters
 * (EOP) for the non-rigid Earth. Only terms with periods less than 2 days are considered, since
 * the longer period terms are included in the IAU2000A nutation model. See Chapter 5 of the IERS
 * Conventions 2010. The typical amplitude of librations is tens of micro-arcseconds (&mu;as) in
 * polar offsets _x_ and _y_ and tens of micro-seconds in UT1.
 *
 * Normally, you would need the combined effect from librations and ocean tides together to apply
 * diurnal corrections to the Earth orientation parameters (EOP) published by IERS. For that,
 * `novas_diurnal_eop()` or `novas_diurnal_eop_at_time()` is more directly suited.
 *
 * REFERENCES:
 * <ol>
 * <li>IERS Conventions 2010, Chapter 5, Tablea 5.1a and 5.1b, see
 * https://iers-conventions.obspm.fr/content/chapter5/icc5.pdf</li>
 * </ol>
 *
 * @param gmst        [h] Greenwich Mean Sidereal Time of observation, e.g. as obtained by
 *                    `novas_gmst()`.
 * @param delaunay    [rad] Delaunay arguments, e.g. as returned by `fund_args()`.
 * @param[out] dxp    [arcsec] x-pole correction due to librations, or NULL if not required.
 * @param[out] dyp    [arcsec] y-pole correction due to librations, or NULL if not required.
 * @param[out] dut1   [s] UT1 correction due to librations, or NULL if not required.
 * @return            0 if successful, or else -1 if the delaunay arguments are NULL (errno set to
 *                    EINVAL).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_diurnal_eop(), novas_diurnal_ocean_tides()
 */
int novas_diurnal_libration(double gmst, const novas_delaunay_args *restrict delaunay, double *restrict dxp, double *restrict dyp,
        double *restrict dut1) {
  if(!delaunay)
    return novas_error(-1, EINVAL, "novas_diurnal_libration", "Delaunay arguments pointer is NULL.");

  if(dxp || dyp) sum_eop_terms(polar_libration_terms, sizeof(polar_libration_terms) / sizeof(novas_eop_terms), gmst, delaunay, dxp, dyp);
  if(dut1) sum_eop_terms(dut1_libration_terms, sizeof(dut1_libration_terms) / sizeof(novas_eop_terms), gmst, delaunay, dut1, NULL);

  return 0;
}

/**
 * Calculate corrections to the Earth orientation parameters (EOP) due to the ocean tides. See
 * Chapter 8 of the IERS Conventions 2010. Ocean tides manifest at the sub-milliarcsecond level in
 * polar offsets _x_ and _y_ and sub-millisecond level in UT1.
 *
 * Normally, you would need the combined effect from librations and ocean tides together to apply
 * diurnal corrections to the Earth orientation parameters (EOP) published by IERS. For that,
 * `novas_diurnal_eop()` or `novas_diurnal_eop_at_time()` is more directly suited.
 *
 * REFERENCES:
 * <ol>
 * <li>IERS Conventions 2010, Chapter 8, Tables 8.2a, 8.2b, 8.3a, and 8.3b, see
 * https://iers-conventions.obspm.fr/content/chapter8/icc8.pdf</li>
 * </ol>
 *
 * @param gmst        [h] Greenwich Mean Sidereal Time of observation, e.g. as obtained by
 *                    `novas_gmst()`.
 * @param delaunay    [rad] Delaunay arguments, e.g. as returned by `fund_args()`.
 * @param[out] dxp    [arcsec] x-pole correction due to ocean tides, or NULL if not required.
 * @param[out] dyp    [arcsec] y-pole correction due to ocean tides, or NULL if not required.
 * @param[out] dut1   [s] UT1 correction due to ocean tides, or NULL if not required.
 * @return            0 if successful, or else -1 if the delaunay arguments are NULL (errno set to
 *                    EINVAL).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_diurnal_eop(), novas_diurnal_librations()
 */
int novas_diurnal_ocean_tides(double gmst, const novas_delaunay_args *restrict delaunay, double *restrict dxp, double *restrict dyp,
        double *restrict dut1) {
  if(!delaunay)
    return novas_error(-1, EINVAL, "novas_diurnal_ocean_tides", "Delaunay arguments pointer is NULL.");

  if(dxp || dyp) sum_eop_terms(polar_tidal_terms, sizeof(polar_tidal_terms) / sizeof(novas_eop_terms), gmst, delaunay, dxp, dyp);
  if(dut1) sum_eop_terms(dut1_tidal_terms, sizeof(dut1_tidal_terms) / sizeof(novas_eop_terms), gmst, delaunay, dut1, NULL);

  return 0;
}

/**
 * Add corrections to the Earth orientation parameters (EOP) due to short term (diurnal and
 * semidiurnal) libration and the ocean tides. See Chapters 5 and 8 of the IERS Conventions. These
 * corrections are typically at the sub-milliarcsecond level in polar offsets _x_ and _y_ and
 * sub-millisecond level in UT1. Thus, these diurnal and semi-diurnal variations are important for
 * the highest precision astrometry only, when converting between ITRS and TIRS frames.
 *
 * REFERENCES:
 * <ol>
 * <li>IERS Conventions 2010, Chapter 5, https://iers-conventions.obspm.fr/content/chapter5/icc5.pdf</li>
 * <li>IERS Conventions 2010, Chapter 8, https://iers-conventions.obspm.fr/content/chapter8/icc8.pdf</li>
 * </ol>
 *
 * @param gmst           [h] Greenwich Mean Sidereal Time of observation, e.g. as obtained by
 *                       `novas_gmst()`.
 * @param delaunay       [rad] Delaunay arguments, e.g. as returned by `fund_args()`.
 * @param[in,out] xp     [arcsec] x-pole: IERS published value (input); corrected for libration
 *                       and ocean tides (output), or NULL if not required.
 * @param[in,out] yp     [arcsec] y-pole: IERS published value (input); corrected for libration
 *                       and ocean tides (output), or NULL if not required.
 * @param[in,out] dut1   [s] UT1: IERS published value (input); corrected for libration and ocean
 *                       tides (output), or NULL if not required.
 * @return               0 if successful, or else -1 if the delaunay arguments are NULL (errno set
 *                       to EINVAL).
 *
 */
static int add_diurnal_eop(double gmst, const novas_delaunay_args *restrict delaunay, double *restrict xp, double *restrict yp,
        double *restrict dut1) {
  double x = 0.0, y = 0.0, ut1 = 0.0;
  double *px = xp ? &x : NULL;
  double *py = yp ? &y : NULL;
  double *put1 = dut1 ? &ut1 : NULL;

  if(!delaunay)
    return novas_error(-1, EINVAL, "add_diurnal_eop", "Delaunay arguments pointer is NULL.");

  novas_diurnal_libration(gmst, delaunay, px, py, put1);
  if(xp)
    *xp += x;
  if(yp)
    *yp += y;
  if(dut1)
    *dut1 += ut1;

  novas_diurnal_ocean_tides(gmst, delaunay, px, py, put1);
  if(xp)
    *xp += x;
  if(yp)
    *yp += y;
  if(dut1)
    *dut1 += ut1;

  return 0;
}

/**
 * Calculate corrections to the Earth orientation parameters (EOP) due to short term (diurnal and
 * semidiurnal) libration and the ocean tides. See Chapters 5 and 8 of the IERS Conventions. This
 * one is faster than `novas_diurnal_eop_at_time()` if the GMST and/or fundamental arguments are
 * readily avaialbe, e.g. from a previous calculation.
 *
 * These corrections are typically at the sub-milliarcsecond level in polar offsets _x_ and _y_
 * and sub-millisecond level in UT1. Thus, these diurnal and semi-diurnal variations are important
 * for the highest precision astrometry only, when converting between ITRS and TIRS frames.
 *
 * REFERENCES:
 * <ol>
 * <li>IERS Conventions 2010, Chapter 5, https://iers-conventions.obspm.fr/content/chapter5/icc5.pdf</li>
 * <li>IERS Conventions 2010, Chapter 8, https://iers-conventions.obspm.fr/content/chapter8/icc8.pdf</li>
 * </ol>
 *
 * @param gmst         [h] Greenwich Mean Sidereal Time of observation, e.g. as obtained by
 *                     `novas_gmst()`.
 * @param delaunay     [rad] Delaunay arguments, e.g. as returned by `fund_args()`.
 * @param[out] dxp     [arcsec] x-pole correction for libration and ocean tides, or NULL if not
 *                     required.
 * @param[out] dyp     [arcsec] y-pole corrections for libration and ocean tides, or NULL if not
 *                     required.
 * @param[out] dut1    [s] UT1 correction for libration and ocean tides, or NULL if not required.
 * @return             0 if successful, or else -1 if the delaunay arguments are NULL (errno set
 *                     to EINVAL).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_diurnal_eop_at_time()
 * @sa novas_diurnal_librations(), novas_diurnal_ocean_tides()
 */
int novas_diurnal_eop(double gmst, const novas_delaunay_args *restrict delaunay, double *restrict dxp, double *restrict dyp,
        double *restrict dut1) {
  if(dxp)
    *dxp = 0.0;
  if(dyp)
    *dyp = 0.0;
  if(dut1)
    *dut1 = 0.0;

  prop_error("novas_diurnal_eop", add_diurnal_eop(gmst, delaunay, dxp, dyp, dut1), 0);
  return 0;
}

/**
 * Calculate corrections to the Earth orientation parameters (EOP) due to short term (diurnal and
 * semidiurnal) libration and the ocean tides at a given astromtric time. See Chapters 5 and 8 of
 * the IERS Conventions.
 *
 * REFERENCES:
 * <ol>
 * <li>IERS Conventions 2010, Chapter 5, https://iers-conventions.obspm.fr/content/chapter5/icc5.pdf</li>
 * <li>IERS Conventions 2010, Chapter 8, https://iers-conventions.obspm.fr/content/chapter8/icc8.pdf</li>
 * </ol>
 *
 * @param time    Astrometric time specification
 * @param dxp     [arcsec] x-pole correction for libration and ocean tides, or NULL if not
 *                required.
 * @param dyp     [arcsec] y-pole corrections for libration and ocean tides, or NULL if not
 *                required.
 * @param dut1    [s] UT1 correction for libration and ocean tides, or NULL if not required.
 * @return        0 if successful, or else -1 if the delaunay arguments are NULL (errno set to
 *                EINVAL).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_diurnal_eop()
 */
int novas_diurnal_eop_at_time(const novas_timespec *restrict time, double *restrict dxp, double *restrict dyp, double *restrict dut1) {
  novas_delaunay_args a = {0, 0, 0, 0, 0};

  if(!time)
    return novas_error(-1, EINVAL, "novas_diurnal_eop_at_time", "time argument is NULL");

  fund_args((time->ijd_tt - NOVAS_JD_J2000 + time->fjd_tt) / JULIAN_CENTURY_DAYS, &a);

  return novas_diurnal_eop(novas_time_gst(time, NOVAS_REDUCED_ACCURACY), &a, dxp, dyp, dut1);
}

/**
 * Corrects a vector in the ITRS (rotating Earth-fixed system) for polar motion, and also
 * corrects the longitude origin (by a tiny amount) to the Terrestrial Intermediate Origin
 * (TIO).  The ITRS vector is thereby transformed to the Terrestrial Intermediate Reference
 * System (TIRS), or equivalently the Pseudo Earth Fixed (PEF), based on the true (rotational)
 * equator and TIO; or vice versa.  Because the true equator is the plane orthogonal to the
 * direction of the Celestial Intermediate Pole (CIP), the components of the output vector are
 * referred to _z_ and _x_ axes toward the CIP and TIO, respectively.
 *
 * NOTES:
 * <ol>
 * <li>
 *  You may obtain Earth orientation parameters from the IERS site and Bulletins. For sub-mas
 *  accuracy you should augment the (interpolated) published values for diurnal and semi-diurnal
 *  librations and the effect of ocean tides, e.g. by using `novas_diurnal_eop()` or
 *  `novas_diurnal_eop_at_time()`.
 * </li>
 * <li>
 *  Generally, this function should not be called if global pole offsets were set via
 *  `cel_pole()` and then used via `place()` or one of its variants to calculate Earth orientation
 *  corrected (TOD or CIRS) apparent coordinates. In such cases, calling `wobble()` would apply
 *  duplicate corrections. It is generally best to forgo using `cel_pole()` going forward, and
 *  instead apply Earth orinetation corrections with `wobble()` only when converting vectors
 *  between the Earth-fixed ITRS and TIRS frames.
 * </li>
 * <li>
 *  The Earth orientation parameters xp, yp should be provided in the same ITRF realization as
 *  the observer location for an Earth-based observer. You can use `novas_itrf_transform_eop()` to
 *  convert the EOP values as necessary.
 * </li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 * <li>Kaplan, G. H. et. al. (1989). Astron. Journ. 97, 1197-1210.</li>
 * <li>Lambert &amp; Bizouard (2002), Astronomy and Astrophysics 394, 317-321.</li>
 * <li>IERS Conventions 2010, Chapter 5, https://iers-conventions.obspm.fr/content/chapter5/icc5.pdf</li>
 * <li>IERS Conventions 2010, Chapter 8, https://iers-conventions.obspm.fr/content/chapter8/icc8.pdf</li>
 * </ol>
 *
 * @param jd_tt         [day] Terrestrial Time (TT) based Julian date.
 * @param direction     WOBBLE_ITRS_TO_TIRS (0) or WOBBLE_TIRS_TO_ITRS (1) to include corrections
 *                      for the TIO's longitude (IAU 2006 method); or else
 *                      WOBBLE_ITRS_TO_PEF (2) or WOBBLE_PEF_TO_ITRS (3) to correct for dx, dy but
 *                      not for the TIO's longitude (old, pre IAU 2006 method). Negative values
 *                      default to WOBBLE_TIRS_TO_ITRS.
 * @param xp            [arcsec] Conventionally-defined X coordinate of Celestial Intermediate
 *                      Pole with respect to ITRS pole. As measured or else the
 *                      interpolated published value from IERS, possibly augmented for diurnal
 *                      variations caused by librations ocean tides if precision below the
 *                      milliarcsecond level is required.
 * @param yp            [arcsec] Conventionally-defined Y coordinate of Celestial Intermediate
 *                      Pole with respect to ITRS pole, in arcseconds. As measured or else the
 *                      interpolated published value from IERS, possibly augmented for diurnal
 *                      variations caused by librations ocean tides if precision below the
 *                      milliarcsecond level is required.
 * @param in            Input position vector, geocentric equatorial rectangular coordinates,
 *                      in the original system defined by 'direction'
 * @param[out] out      Output Position vector, geocentric equatorial rectangular coordinates,
 *                      in the final system defined by 'direction'. It can be the same vector
 *                      as the input.
 *
 * @return              0 if successful, or -1 if the direction is invalid output vector argument is NULL.
 *
 * @sa novas_diurnal_eop(), novas_diurnal_eop_at_time(), novas_itrf_transfor_eop()
 */
int wobble(double jd_tt, enum novas_wobble_direction direction, double xp, double yp, const double *in, double *out) {
  static const char *fn = "wobble";

  double s1 = 0.0;

  if((short) direction < 0)
    direction = 1;
  else if(direction >= NOVAS_WOBBLE_DIRECTIONS)
    return novas_error(-1, EINVAL, fn, "invalid direction: %d", direction);

  if(!in || !out)
    return novas_error(-1, EINVAL, fn, "NULL input or output 3-vector: in=%p, out=%p", in, out);

  xp *= ARCSEC;
  yp *= ARCSEC;

  // Compute approximate longitude of TIO (s'), using eq. (10) of the second reference
  if(direction == WOBBLE_ITRS_TO_TIRS || direction == WOBBLE_TIRS_TO_ITRS) {
    double t = (jd_tt - JD_J2000) / JULIAN_CENTURY_DAYS;
    s1 = -47.0e-6 * ARCSEC * t;
  }

  // Compute elements of rotation matrix.
  // Equivalent to R3(-s')R2(x)R1(y) as per IERS Conventions (2003).
  if(direction == WOBBLE_ITRS_TO_TIRS || direction == WOBBLE_ITRS_TO_PEF) {
    double y = in[1];
    novas_tiny_rotate(in, -yp, -xp, s1, out);
    // Second-order correction for the non-negligible xp, yp product...
    out[0] += xp * yp * y;
  }
  else {
    double x = in[0];
    novas_tiny_rotate(in, yp, xp, -s1, out);
    out[1] += xp * yp * x;
  }

  return 0;
}

/**
 * Computes the geocentric GCRS position and velocity of an observer.
 *
 * @param jd_tt       [day] Terrestrial Time (TT) based Julian date.
 * @param ut1_to_tt   [s] TT - UT1 time difference in seconds
 * @param accuracy    NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param obs         Observer location
 * @param[out] pos    [AU] Position 3-vector of observer, with respect to origin at geocenter,
 *                    referred to GCRS axes, components in AU. (It may be NULL if not required.)
 * @param[out] vel    [AU/day] Velocity 3-vector of observer, with respect to origin at geocenter,
 *                    referred to GCRS axes, components in AU/day. (It must be distinct from the
 *                    pos output vector, and may be NULL if not required)
 * @return            0 if successful, -1 if the 'obs' is NULL or the two output vectors are
 *                    the same, or else 1 if 'accuracy' is invalid, or 2 if 'obserrver->where' is
 *                    invalid.
 *
 * @sa novas_make_frame(), make_observer(), get_ut1_to_tt()
 */
short geo_posvel(double jd_tt, double ut1_to_tt, enum novas_accuracy accuracy, const observer *restrict obs,
        double *restrict pos, double *restrict vel) {
  static const char *fn = "geo_posvel";

  double pos1[3], vel1[3];

  if(!obs)
    return novas_error(-1, EINVAL, fn, "NULL observer location pointer");

  if(pos == vel)
    return novas_error(-1, EINVAL, fn, "identical output pos and vel 3-vectors @ %p", pos);

  // Invalid value of 'accuracy'.
  if(accuracy != NOVAS_FULL_ACCURACY && accuracy != NOVAS_REDUCED_ACCURACY)
    return novas_error(1, EINVAL, fn, "invalid accuracy: %d", accuracy);

  switch(obs->where) {

    case NOVAS_OBSERVER_AT_GEOCENTER:                   // Observer at geocenter.  Trivial case.
      if(pos)
        memset(pos, 0, XYZ_VECTOR_SIZE);
      if(vel)
        memset(vel, 0, XYZ_VECTOR_SIZE);
      return 0;

      // Other two cases: Get geocentric position and velocity vectors of
      // observer wrt equator and equinox of date.

    case NOVAS_OBSERVER_ON_EARTH: {                     // Observer on surface of Earth.
      // Compute UT1 and sidereal time.
      double jd_ut1 = jd_tt - (ut1_to_tt / DAY);

      // Function 'terra' does the hard work, given sidereal time.
      terra(&obs->on_surf, novas_gast(jd_ut1, ut1_to_tt, accuracy), pos1, vel1);
      break;
    }

    case NOVAS_OBSERVER_IN_EARTH_ORBIT: {               // Observer on near-earth spacecraft.
      const double kms = DAY / AU_KM;
      int i;

      // Convert units to AU and AU/day.
      for(i = 3; --i >= 0;) {
        pos1[i] = obs->near_earth.sc_pos[i] / AU_KM;
        vel1[i] = obs->near_earth.sc_vel[i] * kms;
      }

      break;
    }

    case NOVAS_AIRBORNE_OBSERVER: {                     // Airborne observer
      const double kms = DAY / AU_KM;
      observer surf = *obs;
      int i;

      surf.where = NOVAS_OBSERVER_ON_EARTH;

      // Get the stationary observer velocity at the location
      prop_error(fn, geo_posvel(jd_tt, ut1_to_tt, accuracy, &surf, pos1, vel1), 0);

      // Add in the aircraft motion
      for(i = 3; --i >= 0;)
        vel1[i] = novas_add_vel(vel1[i], obs->near_earth.sc_vel[i] * kms);

      break;
    }

    case NOVAS_SOLAR_SYSTEM_OBSERVER: {               // Observer in Solar orbit
      const object earth = NOVAS_EARTH_INIT;
      const double tdb[2] = { jd_tt, tt2tdb(jd_tt) / DAY };
      int i;


      // Get the position and velocity of the geocenter rel. to SSB
      prop_error(fn, ephemeris(tdb, &earth, NOVAS_BARYCENTER, accuracy, pos1, vel1), 0);

      // Return velocities w.r.t. the geocenter.
      for(i = 3; --i >= 0;) {
        if(pos)
          pos[i] = obs->near_earth.sc_pos[i] - pos1[i];
        if(vel)
          vel[i] = novas_add_vel(obs->near_earth.sc_vel[i], -vel1[i]);
      }

      // Already in GCRS...
      return 0;
    }

    default:
      return novas_error(2, EINVAL, fn, "invalid observer type (where): %d", obs->where);
  }

  // For these calculations we can assume TDB = TT (< 2 ms difference)...

  // Transform geocentric position vector of observer to GCRS.
  if(pos)
    tod_to_gcrs(jd_tt, accuracy, pos1, pos); // Use TT for TDB

  // Transform geocentric velocity vector of observer to GCRS.
  if(vel)
    tod_to_gcrs(jd_tt, accuracy, vel1, vel); // Use TT for TDB

  return 0;
}

/**
 * Determines the angle of an object above or below the Earth's limb (horizon).  The geometric
 * limb is computed, assuming the Earth to be an airless sphere (no refraction or oblateness
 * is included).  The observer can be on or above the Earth.  For an observer on the surface
 * of the Earth, this function returns the approximate unrefracted elevation.
 *
 * @param pos_src         [AU] Position 3-vector of observed object, with respect to origin at
 *                        geocenter, components in AU.
 * @param pos_obs         [AU] Position 3-vector of observer, with respect to origin at
 *                        geocenter, components in AU.
 * @param[out] limb_ang   [deg] Angle of observed object above (+) or below (-) limb in degrees,
 *                        or NAN if reurning with an error. It may be NULL if not required.
 * @param[out] nadir_ang  Nadir angle of observed object as a fraction of apparent radius
 *                        of limb: %lt;1.0 if below the limb; 1.0 on the limb; or &gt;1.0 if
 *                        above the limb. Returns NAN in case of an error return. It may be NULL
 *                        if not required.
 *
 * @return    0 if successful, or -1 if either of the input vectors is NULL or if either input
 *            position is a null vector (at the geocenter).
 *
 */
int limb_angle(const double *pos_src, const double *pos_obs, double *restrict limb_ang, double *restrict nadir_ang) {
  static const char *fn = "limb_angle";
  double d_src, d_obs, aprad, zdlim, coszd, zdobj;

  // Default return values (in case of error)
  if(limb_ang)
    *limb_ang = NAN;
  if(nadir_ang)
    *nadir_ang = NAN;

  if(!pos_src || !pos_obs)
    return novas_error(-1, EINVAL, fn, "NULL input 3-vector: pos_src=%p, pos_obs=%p", pos_src, pos_obs);

  // Compute the distance to the object and the distance to the observer.
  d_src = novas_vlen(pos_src);
  d_obs = novas_vlen(pos_obs);

  if(!d_src)
    return novas_error(-1, EINVAL, fn, "looking at geocenter");

  if(!d_obs)
    return novas_error(-1, EINVAL, fn, "observer is at geocenter");

  // Compute apparent angular radius of Earth's limb.
  aprad = (d_obs >= ERAD_AU) ? asin(ERAD_AU / d_obs) : HALF_PI;

  // Compute zenith distance of Earth's limb.
  zdlim = M_PI - aprad;

  // Compute zenith distance of observed object.
  coszd = novas_vdot(pos_src, pos_obs) / (d_src * d_obs);

  if(coszd <= -1.0)
    zdobj = M_PI;
  else if(coszd >= 1.0)
    zdobj = 0.0;
  else
    zdobj = acos(coszd);

  // Angle of object wrt limb is difference in zenith distances.
  if(limb_ang)
    *limb_ang = (zdlim - zdobj) / DEGREE;

  // Nadir angle of object as a fraction of angular radius of limb.
  if(nadir_ang)
    *nadir_ang = (M_PI - zdobj) / aprad;

  return 0;
}


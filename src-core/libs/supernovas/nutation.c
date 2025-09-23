/**
 * @file
 *
 * @author G. Kaplan and A. Kovacs
 *
 *  SuperNOVAS implementations for the IAU2000 nutation series calculations, with varying
 *  trade-offs between computational cost and precision. It provides support for both the
 *  IAU 2000A, a truncated version with ~1 mas precision in the cureent epoch, and a
 *  NOVAS-specific reduced precision series with intermediate accuracy.
 *
 *  Based on the NOVAS C Edition, Version 3.1:
 *
 *  U. S. Naval Observatory<br>
 *  Astronomical Applications Dept.<br>
 *  Washington, DC<br>
 *  <a href="http://www.usno.navy.mil/USNO/astronomical-applications">
 *  http://www.usno.navy.mil/USNO/astronomical-applications</a>
 */

#include <stdint.h>
#include <errno.h>

/// \cond PRIVATE
#define __NOVAS_INTERNAL_API__    ///< Use definitions meant for internal use by SuperNOVAS only
#include "novas.h"

#define T0        NOVAS_JD_J2000

/**
 * Data structure to contain coefficients for a single periodic nutation term.
 *
 */
typedef struct {
  int32_t A;         ///< [10 nas] Sine term coefficient
  int32_t B;         ///< [10 nas] Cosine term coefficient
  int8_t n[14];      ///< multiples (5 fund args + 8 planets [Mercury -- Neptune] + accumulated precession)
  int8_t from;       ///< index of first non-zero multiple in n[].
  int8_t to;         ///< index after the last non-zero multiple in n[].
} nutation_terms;

#include "nutation/iau2006a.tab.h"
#include "nutation/iau2006b.tab.h"
#include "nutation/nu2000k.tab.h"

/// \endcond

/**
 * Returns the IAU2000 / 2006 values for nutation in longitude and nutation in obliquity for a given TDB
 * Julian date and the desired level of accuracy. For NOVAS_FULL_ACCURACY (0), the IAU 2000A R06
 * nutation model is used. Otherwise, the model set by set_nutation_lp_provider() is used, or else
 * the iau2000b() by default.
 *
 * REFERENCES:
 * <ol>
 * <li>Kaplan, G. (2005), US Naval Observatory Circular 179.</li>
 * <li>Capitaine, N., P.T. Wallace and J. Chapront (2005), “Improvement of the IAU 2000 precession
 *     model.” Astronomy &amp; Astrophysics, Vol. 432, pp. 355–67.</li>
 * <li>Coppola, V., Seago, G.H., &amp; Vallado, D.A. (2009), AAS 09-159</li>
 * </ol>
 *
 * @param t           [cy] TDB time in Julian centuries since J2000.0
 * @param accuracy    NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] dpsi   [arcsec] Nutation in longitude in arcseconds.
 * @param[out] deps   [arcsec] Nutation in obliquity in arcseconds.
 *
 * @return            0 if successful, or -1 if the output pointer arguments are NULL
 *
 * @sa nutation(), set_nutation_lp_provider()
 * @sa iau2000a(), iau2000b(), nu2000k()
 * @sa NOVAS_JD_J2000
 */
int nutation_angles(double t, enum novas_accuracy accuracy, double *restrict dpsi, double *restrict deps) {
  static THREAD_LOCAL double last_t = NAN, last_dpsi, last_deps;
  static THREAD_LOCAL enum novas_accuracy last_acc = -1;

  if(!dpsi || !deps) {
    if(dpsi)
      *dpsi = NAN;
    if(deps)
      *deps = NAN;

    return novas_error(-1, EINVAL, "nutation_angles", "NULL output pointer: dspi=%p, deps=%p", dpsi, deps);
  }

  if(!(fabs(t - last_t) < 1e-12) || (accuracy != last_acc)) {
    novas_nutation_provider nutate_call = (accuracy == NOVAS_FULL_ACCURACY) ? iau2000a : get_nutation_lp_provider();
    nutate_call(JD_J2000, t * JULIAN_CENTURY_DAYS, &last_dpsi, &last_deps);

    // Convert output to arcseconds.
    last_dpsi /= ARCSEC;
    last_deps /= ARCSEC;

    last_acc = accuracy;
    last_t = t;
  }

  *dpsi = last_dpsi;
  *deps = last_deps;

  return 0;
}

static double sum_terms(double t, const double *a, const nutation_terms *P0, int N0, const nutation_terms *P1, int N1) {
  double sum = 0.0;
  int i;

  for(i = N0; --i >= 0; ) {
    const nutation_terms *T = &P0[i];
    double arg = 0.0;
    int k;

    for(k = T->from; k < T->to; k++)
      arg += T->n[k] * a[k];

    sum += T->A * sin(arg) + T->B * cos(arg);
  }

  for(i = N1; --i >= 0; ) {
    const nutation_terms *T = &P1[i];
    double arg = 0.0;
    int k;

    for(k = T->from; k < T->to; k++)
      arg += T->n[k] * a[k];

    sum += (T->A * sin(arg) + T->B * cos(arg)) * t;
  }

  return sum * 1e-8 * ARCSEC;
}

static int iau2006_fp(double jd_tt_high, double jd_tt_low, int nA0, int nA1, int nB0, int nB1, double *restrict dpsi, double *restrict deps) {
  // Interval between fundamental epoch J2000.0 and given date.
  const double t = ((jd_tt_high - T0) + jd_tt_low) / JULIAN_CENTURY_DAYS;

  double a[14] = {0.0};
  int i;

  // Compute fundamental arguments from Simon et al. (1994), in radians.
  fund_args(t, (novas_delaunay_args *) a);

  // Planetary longitudes, Mercury through Neptune, wrt mean dynamical
  // ecliptic and equinox of J2000
  // (Simon et al. 1994, 5.8.1-5.8.8).
  for(i = NOVAS_MERCURY; i <= NOVAS_NEPTUNE; i++)
    a[4 + i] = planet_lon(t, i);

  // General precession in longitude (Simon et al. 1994), equivalent
  // to 5028.8200 arcsec/cy at J2000.
  a[13] = accum_prec(t);

  if(dpsi)
    *dpsi = sum_terms(t, a, A0, nA0, A1, nA1);

  if(deps)
    *deps = sum_terms(t, a, B0, nB0, B1, nB1);

  return 0;
}

/**
 * Computes the IAU 2000A high-precision nutation series for the specified date, to 0.1 &mu;as
 * accuracy. It is rather expensive computationally.
 *
 * NOTES:
 * <ol>
 * <li>As of SuperNOVAS v1.4.2, this function has been modified to replace the original IAU2000A
 * model coefficients with the IAU2006 (a.k.a. IAU2000A R06) model coefficients, to provide an
 * updated nutation model, which is dynamically consistent with the IAU2006 (P03) precesion model
 * of Capitaine et al. 2003. This is now the same model with respect to which the IERS Earth
 * orinetation parameters are computed and published.
 * </ol>
 *
 * REFERENCES:
 * <ol>
 *  <li>IERS Conventions (2003), Chapter 5.</li>
 *  <li>Simon et al. (1994) Astronomy and Astrophysics 282, 663-683, esp. Sections 3.4-3.5.</li>
 *  <li>Capitaine, N. et al. (2003), Astronomy And Astrophysics 412, pp. 567-586.</li>
 *  <li>https://hpiers.obspm.fr/eop-pc/models/nutations/nut.html</li>
 * </ol>
 *
 * @param jd_tt_high  [day] High-order part of the Terrestrial Time (TT) based Julian date.
 *                    Typically it may be the integer part of a split date for the highest
 *                    precision, or the full date for normal (reduced) precision.
 * @param jd_tt_low   [day] Low-order part of the Terrestrial Time (TT) based Julian date.
 *                    Typically, it may be the fractional part of a split date for the highest
 *                    precision, or 0.0 for normal (reduced) precision.
 * @param[out] dpsi   [rad] &delta;&psi; Nutation (luni-solar + planetary) in longitude. It may be
 *                    NULL if not required.
 * @param[out] deps   [rad] &delta;&epsilon; Nutation (luni-solar + planetary) in obliquity. It
 *                    may be NULL if not required.
 * @return            0
 *
 * @sa iau2000b(), nu2000k(), nutation_angles(), nutation()
 */
int iau2000a(double jd_tt_high, double jd_tt_low, double *restrict dpsi, double *restrict deps) {
  prop_error("iau200a", iau2006_fp(jd_tt_high, jd_tt_low, 1320, 38, 1037, 19, dpsi, deps), 0);
  return 0;
}

/**
 * Computes the forced nutation of the non-rigid Earth based at reduced precision. It reproduces
 * the IAU 2000A (R06) model to a precision of 1 milliarcsecond in the interval 1995-2020, while
 * being about 15x faster than `iau2000a()`.
 *
 * NOTES
 * <ol>
 * <li>Originally this was the IAU2000B series of McCarthy &amp; Luzum (2003), consistent with the
 * original IAU2000 precession model</li>
 *
 * <li>As of SuperNOVAS v1.4.2, this function has been modified to use a truncated series for the
 * IAU2006 (a.k.a. IAU 2000A R06) nutation model, whereby terms with amplitudes larger than 100
 * &mu;as are omitted, resulting in 102 terms for the longitude and 57 terms the obliquity. This
 * results in similar, or slightly better, precision than the original IAU2000B series of McCarthy
 * &amp; Luzum (2003), and it is now dynamically consistent with the IAU2006 (P03) precession
 * model (Capitaine et al. 2005).</li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 * <li>McCarthy, D. and Luzum, B. (2003). "An Abridged Model of the
 *     Precession &amp; Nutation of the Celestial Pole," Celestial
 *     Mechanics and Dynamical Astronomy, Volume 85, Issue 1,
 *     Jan. 2003, p. 37. (IAU 2000B) IERS Conventions (2003), Chapter 5.</li>
 * <li>Capitaine, N. et al. (2003), Astronomy And Astrophysics 412, pp. 567-586.</li>
 * </ol>
 *
 * @param jd_tt_high  [day] High-order part of the Terrestrial Time (TT) based Julian date.
 *                    Typically it may be the integer part of a split date for the highest
 *                    precision, or the full date for normal (reduced) precision.
 * @param jd_tt_low   [day] Low-order part of the Terrestrial Time (TT) based Julian date.
 *                    Typically it may be the fractional part of a split date for the highest
 *                    precision, or 0.0 for normal (reduced) precision.
 * @param[out] dpsi   [rad] &delta;&psi; Nutation (luni-solar + planetary) in longitude, It may be
 *                    NULL if not required.
 * @param[out] deps   [rad] &delta;&epsilon; Nutation (luni-solar + planetary) in obliquity. It
 *                    may be NULL if not required.
 * @return            0
 *
 * @sa iau2000a(), nu2000k(), nutation_angles(), set_nutation_lp_provider(), nutation()
 */
int iau2000b(double jd_tt_high, double jd_tt_low, double *restrict dpsi, double *restrict deps) {
  prop_error("iau2000b", iau2006_fp(jd_tt_high, jd_tt_low, 98, 4, 55, 2, dpsi, deps), 0);
  return 0;
}

/**
 * Computes the forced nutation of the non-rigid Earth: Model NU2000K.  This model is a modified
 * version of the original IAU 2000A, which has been truncated for speed of execution. NU2000K
 * agrees with IAU 2000A at the 0.1 milliarcsecond level from 1700 to 2300, while being is about
 * 5x faster than the more precise `iau2000a()`.
 *
 * NU2000K was compared to IAU 2000A over six centuries (1700-2300). The average error in d&psi;
 * is 20 microarcseconds, with 98% of the errors < 60 microarcseconds;  the average error in
 * d&epsilon;is 8 microarcseconds, with 100% of the errors < 60 microarcseconds.
 *
 * NU2000K was developed by G. Kaplan (USNO) in March 2004
 *
 * NOTES:
 * <ol>
 * <li>As of SuperNOVAS v1.4.2, this function has been modified to include a rescaling of the
 * original IAU2000 nutation compatible values to 'IAU2006' (i.e. IAU2000A R06) compatible values,
 * according to Coppola, Seago, and Vallado (2009). The rescaling makes this model more
 * dynamically consistent with the IAU2006 (P03) precession model of Capitaine et al. (2003).
 * </li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 * <li>IERS Conventions (2003), Chapter 5.</li>
 * <li>Simon et al. (1994) Astronomy and Astrophysics 282, 663-683, esp. Sections 3.4-3.5.</li>
 * <li>Capitaine, N. et al. (2003), Astronomy And Astrophysics 412, pp. 567-586.</li>
 * <li>Coppola, V., Seago, G.H., &amp; Vallado, D.A. (2009), AAS 09-159</li>
 * </ol>
 *
 * @param jd_tt_high  [day] High-order part of the Terrestrial Time (TT) based Julian date.
 *                    Typically it may be the integer part of a split date for the highest
 *                    precision, or the full date for normal (reduced) precision.
 * @param jd_tt_low   [day] Low-order part of the Terrestrial Time (TT) based Julian date.
 *                    Typically it may be the fractional part of a split date for the highest
 *                    precision, or 0.0 for normal (reduced) precision.
 * @param[out] dpsi   [rad] &delta;&psi; Nutation (luni-solar + planetary) in longitude, It may be
 *                    NULL if not required.
 * @param[out] deps   [rad] &delta;&epsilon; Nutation (luni-solar + planetary) in obliquity. It
 *                    may be NULL if not required.
 * @return            0
 *
 * @sa iau2000a(), iau2000b(), nutation_angles(), set_nutation_lp_provider(), nutation()
 */
int nu2000k(double jd_tt_high, double jd_tt_low, double *restrict dpsi, double *restrict deps) {
  // Interval between fundamental epoch J2000.0 and given date.
  const double t = ((jd_tt_high - T0) + jd_tt_low) / JULIAN_CENTURY_DAYS;

  // Planetary longitudes, Mercury through Neptune, wrt mean dynamical
  // ecliptic and equinox of J2000, with high order terms omitted
  // (Simon et al. 1994, 5.8.1-5.8.8).
  //const double alme = planet_lon(t, NOVAS_MERCURY);
  const double alve = planet_lon(t, NOVAS_VENUS);
  const double alea = planet_lon(t, NOVAS_EARTH);
  const double alma = planet_lon(t, NOVAS_MARS);
  const double alju = planet_lon(t, NOVAS_JUPITER);
  const double alsa = planet_lon(t, NOVAS_SATURN);
  //const double alur = planet_lon(t, NOVAS_URANUS);
  //const double alne = planet_lon(t, NOVAS_NEPTUNE);

  // General precession in longitude (Simon et al. 1994), equivalent
  // to 5028.8200 arcsec/cy at J2000.
  const double apa = accum_prec(t);

  // P03 scaling factor from Coppola+2009
  const double fP03 = -2.7774e-6 * t;

  novas_delaunay_args a;

  double dpsils = 0.0, depsls = 0.0, dpsipl = 0.0, depspl = 0.0;

  int i;

  // Compute fundamental arguments from Simon et al. (1994), in radians.
  fund_args(t, &a);

  // ** Luni-solar nutation. **
  // Summation of luni-solar nutation series (in reverse order).
  for(i = 323; --i >= 0;) {
    const int8_t *n = &nals_t[i][0];
    const int32_t *c = &cls_t[i][0];

    double arg = 0.0, sarg, carg;

    if(n[0])
      arg += n[0] * a.l;
    if(n[1])
      arg += n[1] * a.l1;
    if(n[2])
      arg += n[2] * a.F;
    if(n[3])
      arg += n[3] * a.D;
    if(n[4])
      arg += n[4] * a.Omega;

    sarg = sin(arg);
    carg = cos(arg);

    // Term.
    dpsils += (c[0] + c[1] * t) * sarg + c[2] * carg;
    depsls += (c[3] + c[4] * t) * carg + c[5] * sarg;
  }

  // ** Planetary nutation. **

  // Summation of planetary nutation series (in reverse order).
  for(i = 165; --i >= 0;) {
    const int8_t *n = &napl_t[i][0];
    const int16_t *c = &cpl_t[i][0];

    // Argument and functions.
    double arg = 0.0, sarg, carg;

    if(n[0])
      arg += n[0] * a.l;
    /* This version of Nutation does not contain terms for l1
    if(n[1])
      arg += n[1] * a.l1;
     */
    if(n[2])
      arg += n[2] * a.F;
    if(n[3])
      arg += n[3] * a.D;
    if(n[4])
      arg += n[4] * a.Omega;
    /* This version of Nutation does not contain terms for Mercury
    if(n[5])
      arg += n[5] * alme;
     */
    if(n[6])
      arg += n[6] * alve;
    if(n[7])
      arg += n[7] * alea;
    if(n[8])
      arg += n[8] * alma;
    if(n[9])
      arg += n[9] * alju;
    if(n[10])
      arg += n[10] * alsa;
    /* This version of Nutation does not contain terms for Uranus and Neptune
    if(n[11])
      arg += n[11] * alur;
    if(n[12])
      arg += n[12] * alne;
     */
    if(n[13])
      arg += n[13] * apa;

    sarg = sin(arg);
    carg = cos(arg);

    // Term.
    dpsipl += c[0] * sarg + c[1] * carg;
    depspl += c[2] * sarg + c[3] * carg;
  }

  // Add planetary and luni-solar components, and apply rescaling to IAU2006 equivalent model
  // to provide nutations that is Dynamically consistent with the IAU20006 (P03) precession model
  // (see Coppola, Seago, and Vallado 2009).
  if(dpsi)
    *dpsi = 1e-7 * (1.0000004697 + fP03) * (dpsipl + dpsils) * ARCSEC;
  if(deps)
    *deps = 1e-7 * (1.0 + fP03) * (depspl + depsls) * ARCSEC;

  return 0;
}

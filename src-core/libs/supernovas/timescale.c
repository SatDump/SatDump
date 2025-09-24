/**
 * @file
 *
 * @date Created  on Jun 24, 2024
 * @author Attila Kovacs
 * @since 1.1
 *
 *   A set of SuperNOVAS routines to make handling of astronomical timescales and conversions
 *   among them easier.
 *
 * @sa frames.c, observer.c
 */

/// \cond PRIVATE
#define _GNU_SOURCE                 ///< for strcasecmp()
#define __NOVAS_INTERNAL_API__      ///< Use definitions meant for internal use by SuperNOVAS only
/// \endcond

#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <string.h>
#ifdef _WIN32
#include "libs/strings_h_win.h"
#else
#include <strings.h>              // strcasecmp() / strncasecmp()
#endif
#include <errno.h>
#include <math.h>
#include <ctype.h>                // isspace()

#include "novas.h"

/// \cond PRIVATE
#define DTA         (32.184 / DAY)        ///< [day] TT - TAI time difference
#define GPS2TAI     (19.0 / DAY)          ///< [day] TAI - GPS time difference

#define IDAY        86400                 ///< [s] 1 day

#define IJD_J2000   2451545

#define UNIX_SECONDS_0UTC_1JAN2000  946684800    ///< [s] UNIX time at J2000.0
#define UNIX_J2000                  (UNIX_SECONDS_0UTC_1JAN2000 + (IDAY / 2))

// IAU 2006 Resolution B3
#define TC_T0      2443144.5003725       ///< 1977 January 1, 0h 0m 0s TAI

/**
 * Relative rate at which Barycentric coordinate time progresses fastern than time on Earth i.e.,
 * Terrestrial Time (TT).
 *
 * REFERENCES:
 * <ol>
 * <li>Ryan S. Park et al. 2021 AJ 161 105, DOI 10.3847/1538-3881/abd414</li>
 * </ol>
 */
#define TC_LB      1.550519768e-8

/**
 * Relative rate at which Geocentric coordinate time progresses fastern than time on Earth i.e.,
 * Terrestrial Time (TT).
 *
 * NOTES:
 * <ol>
 * <li>Updated to DE440 value (Park et al. 2021) in v1.4</li>
 * </ol>
 * REFERENCES:
 * <ol>
 * <li>Ryan S. Park et al. 2021 AJ 161 105, DOI 10.3847/1538-3881/abd414</li>
 * </ol>
 */
#define TC_LG      6.969290134e-10

#define TC_TDB0    (6.55e-5 / DAY)       ///< TDB time offset at TC_T0

#define E9          1000000000           ///< 10<sup>9</sup> as integer

#define DATE_SEP_CHARS  "-_./ \t\r\n\f"             ///< characters that may separate date components
#define DATE_SEP        "%*[" DATE_SEP_CHARS "]"    ///< Parse pattern for ignored date separators

/// Parse pattern for month specification, either as a 1-2 digit integer or as a month name or abbreviation.
#define MONTH_SPEC      "%9[^" DATE_SEP_CHARS "]"

#define DAY_MILLIS    86400000L         ///< milliseconds in a day
#define HOUR_MILLIS   3600000L          ///< milliseconds in an hour
#define MIN_MILLIS    60000L            ///< milliseconds in a minute

static const double iM[] = NOVAS_RMASS_INIT;       ///< [1/M<sub>sun</sub>]
static const double R[] = NOVAS_PLANET_RADII_INIT; ///< [m]

/// \endcond

#if __Lynx__ && __powerpc__
// strcasecmp() / strncasecmp() are not defined on PowerPC / LynxOS 3.1
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);
#endif


/**
 * Computes the Terrestrial Time (TT) based Julian date corresponding to a Barycentric
 * Dynamical Time (TDB) Julian date, and retuns th TDB-TT time difference also. It has a
 * maximum error of 10 &mu;s between for dates 1600 and 2200.
 *
 * This function is wrapped by `tt2tdb()`, which is typically a lot easier to use as it
 * returns the time difference directly, which can then be used to convert time in both
 * directions with greater ease. For exable to convert a TT-based date to a TDB-based date:
 *
 * ```c
 *   double TDB = TT + tt2tdb(TT) / 86400.0;
 * ```
 *
 * is equivalent to:
 *
 * ```c
 *   double dt;
 *   tdb2tt(TT, NULL, &dt);   // Uses TT input so don't attempt to calculate TT here...
 *   double TDB = TT + dt / 86400.0;
 * ```
 *
 * NOTES:
 * <ol>
 * <li>This function caches the results of the last calculation in case it may be re-used at
 * no extra computational cost for the next call.</li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 * <li>Fairhead, L. &amp; Bretagnon, P. (1990) Astron. &amp; Astrophys. 229, 240.</li>
 * <li>Kaplan, G. (2005), US Naval Observatory Circular 179.</li>
 * </ol>
 *
 * @param jd_tdb         [day] Barycentric Dynamic Time (TDB) based Julian date.
 * @param[out] jd_tt     [day] Terrestrial Time (TT) based Julian date. (It may be NULL
 *                       if not required)
 * @param[out] secdiff   [s] Difference 'tdb_jd'-'tt_jd', in seconds. (It may be NULL if
 *                       not required)
 * @return               0
 *
 * @sa tt2tdb()
 */
int tdb2tt(double jd_tdb, double *restrict jd_tt, double *restrict secdiff) {
  static THREAD_LOCAL double last_tdb = NAN;
  static THREAD_LOCAL double d;

  if(!novas_time_equals(jd_tdb, last_tdb)) {
    // Expression given in USNO Circular 179, eq. 2.6.
    const double t = (jd_tdb - NOVAS_JD_J2000) / JULIAN_CENTURY_DAYS;
    d = 0.001657 * sin(628.3076 * t + 6.2401) + 0.000022 * sin(575.3385 * t + 4.2970) + 0.000014 * sin(1256.6152 * t + 6.1969)
    + 0.000005 * sin(606.9777 * t + 4.0212) + 0.000005 * sin(52.9691 * t + 0.4444) + 0.000002 * sin(21.3299 * t + 5.5431)
    + 0.000010 * t * sin(628.3076 * t + 4.2490);

    last_tdb = jd_tdb;
  }

  if(jd_tt)
    *jd_tt = jd_tdb - d / DAY;

  if(secdiff)
    *secdiff = d;

  return 0;
}

/**
 * Returns the TDB - TT time difference in seconds for a given TT or TDB date, with a maximum
 * error of 10 &mu;s for dates between 1600 and 2200.
 *
 * REFERENCES
 * <ol>
 * <li>Fairhead, L. &amp; Bretagnon, P. (1990) Astron. &amp; Astrophys. 229, 240.</li>
 * <li>Kaplan, G. (2005), US Naval Observatory Circular 179.</li>
 * <li>Moyer, T.D. (1981), Celestial mechanics, Volume 23, Issue 1, pp. 57-68<.li>
 * </ol>
 *
 * @param jd_tt     [day] Terrestrial Time (TT) based Julian date, but Barycentri Dynamical Time (TDB)
 *                  can also be used here without any loss of precision on the result.
 * @return          [s] TDB - TT time difference.
 *
 * @since 1.0
 * @author Attila Kovacs
 *
 * @sa tt2tdb_hp(), tt2tdb_fp(), tdb2tt()
 */
double tt2tdb(double jd_tt) {
  double dt;

  tdb2tt(jd_tt, NULL, &dt);
  return dt;
}

/**
 * Returns the TDB-TT time difference with flexible precision. This implementation uses the series
 * expansion by Fairhead &amp; Bretagnon 1990, terminating theterm at or below the specified
 * limiting amplitude.
 *
 * REFERENCES:
 * <ol>
 * <li>Fairhead, L., &amp; Bretagnon, P. (1990) A&amp;A, 229, 240</li>
 * </ol>
 *
 * @param jd_tt   [day] Terrestrial Time (TT) based Julian date, but Barycentric Dynamical Time (TDB)
 * @param limit   [us] Amplitude of limiting term to include in series. 0 or negative values will
 *                include all terms, producing the same result as `tt2tdb_hp()`.
 * @return        [s] TDB - TT time difference.
 *
 * @since 1.4
 * @author Attila Kovacs
 *
 * @sa tt2tdb_hp(), tt2tdb()
 */
double tt2tdb_fp(double jd_tt, double limit) {
  static const double a[][3] = { //
          { 1656.674564,    6283.075943033, 6.240054195 }, //
          {   22.417471,    5753.384970095, 4.296977442 }, //
          {   13.839792,   12566.151886066, 6.196904410 }, //
          {    4.770086,     529.690965095, 0.444401603 }, //
          {    4.676740,    6069.776754553, 4.021195093 }, //
          {    2.256707,     213.299095438, 5.543113262 }, //
          {    1.694205,      -3.523118349, 5.025132748 }, //
          {    1.554905,   77713.772618729, 5.198467090 }, //
          {    1.276839,    7860.419392439, 5.988822341 }, //
          {    1.193379,    5223.693919802, 3.649823730 }, //
          {    1.115322,    3930.209696220, 1.422745069 }, //
          {    0.794185,   11506.769769794, 2.322313077 }, //
          {    0.600309,    1577.343542448, 2.678271909 }, //
          {    0.496817,    6208.294251424, 5.696701824 }, //
          {    0.486306,    5884.926846583, 0.520007179 }, //
          {    0.468597,    6244.942814354, 5.866398759 }, //
          {    0.447061,      26.298319800, 3.615796498 }, //
          {    0.435206,    -398.149003408, 4.349338347 }, //
          {    0.432392,      74.781598567, 2.435898309 }, //
          {    0.375510,    5507.553238667, 4.103476804 }, //
          {    0.243085,    -775.522611324, 3.651837925 }, //
          {    0.230685,    5856.477659115, 4.773852582 }, //
          {    0.203747,   12036.460734888, 4.333987818 }, //
          {    0.173435,   18849.227549974, 6.153743485 }, //
          {    0.159080,   10977.078804699, 1.890075226 }, //
          {    0.143935,    -796.298006816, 5.957517795 }, //
          {    0.137927,   11790.629088659, 1.135934669 }, //
          {    0.119979,      38.133035638, 4.551585768 }, //
          {    0.118971,    5486.777843175, 1.914547226 }, //
          {    0.116120,    1059.381930189, 0.873504123 }, //
          {    0.101868,   -5573.142801634, 5.984503847 }, //
          {    0.098358,    2544.314419883, 0.092793886 }, //
          {    0.080164,     206.185548437, 2.095377709 }, //
          {    0.079645,    4694.002954708, 2.949233637 }, //
          {    0.075019,    2942.463423292, 4.980931759 }, //
          {    0.064397,    5746.271337896, 1.280308748 }, //
          {    0.063814,    5760.498431898, 4.167901731 }, //
          {    0.062617,      20.775395492, 2.654394814 }, //
          {    0.058844,     426.598190876, 4.839650148 }, //
          {    0.054139,   17260.154654690, 3.411091093 }, //
          {    0.048373,     155.420399434, 2.251573730 }, //
          {    0.048042,    2146.165416475, 1.495846011 }, //
          {    0.046551,      -0.980321068, 0.921573539 }, //
          {    0.042732,     632.783739313, 5.720622217 }, //
          {    0.042560,  161000.685737473, 1.270837679 }, //
          {    0.042411,    6275.962302991, 2.869567043 }, //
          {    0.040759,   12352.852604545, 3.981496998 }, //
          {    0.040480,   15720.838784878, 2.546610123 }, //
          {    0.040184,      -7.113547001, 3.565975565 }, //
          {    0.036955,    3154.687084896, 5.071801441 }, //
          {    0.036564,    5088.628839767, 3.324679049 }, //
          {    0.036507,     801.820931124, 6.248866009 }, //
          {    0.034867,     522.577418094, 5.210064075 }, //
          {    0.033529,    9437.762934887, 2.404714239 }, //
          {    0.033477,    6062.663207553, 4.144987272 }, //
          {    0.032438,    6076.890301554, 0.749317412 }, //
          {    0.032423,    8827.390269875, 5.541473556 }, //
          {    0.030215,    7084.896781115, 3.389610345 }, //
          {    0.029862,   12139.553509107, 1.770181024 }, //
          {    0.029247,  -71430.695617928, 4.183178762 }, //
          {    0.028244,   -6286.598968340, 5.069663519 }, //
          {    0.027567,    6279.552731642, 5.040846034 }, //
          {    0.025196,    1748.016413067, 2.901883301 }, //
          {    0.024816,   -1194.447010225, 1.087136918 }, //
          {    0.022567,    6133.512652857, 3.307984806 }, //
          {    0.022509,   10447.387839604, 1.460726241 }, //
          {    0.021691,   14143.495242431, 5.952658009 }, //
          {    0.020937,    8429.241266467, 0.652303414 }, //
          {    0.020322,     419.484643875, 3.735430632 }, //
          {    0.017673,    6812.766815086, 3.186129845 }, //
          {    0.017806,      73.297125859, 3.475975097 }, //
          {    0.016155,   10213.285546211, 1.331103168 }, //
          {    0.015974,   -2352.866153772, 6.145309371 }, //
          {    0.015949,    -220.412642439, 4.005298270 }, //
          {    0.015078,   19651.048481098, 3.969480770 }, //
          {    0.014751,    1349.867409659, 4.308933301 }, //
          {    0.014318,   16730.463689596, 3.016058075 }, //
          {    0.014223,   17789.845619785, 2.104551349 }, //
          {    0.013671,    -536.804512095, 5.971672571 }, //
          {    0.012462,     103.092774219, 1.737438797 }, //
          {    0.012420,    4690.479836359, 4.734090399 }, //
          {    0.011942,    8031.092263058, 2.053414715 }, //
          {    0.011847,    5643.178563677, 5.489005403 }, //
          {    0.011707,   -4705.732307544, 2.654125618 }, //
          {    0.011622,    5120.601145584, 4.863931876 }, //
          {    0.010962,       3.590428652, 2.196567739 }, //
          {    0.010825,     553.569402842, 0.842715011 }, //
          {    0.010396,     951.718406251, 5.717799605 }, //
          {    0.010453,    5863.591206116, 1.913704550 }, //
          {    0.010099,     283.859318865, 1.942176992 }, //
          {    0.009858,    6309.374169791, 1.061816410 }, //
          {    0.009963,     149.563197135, 4.870690598 }, //
          {    0.009370,  149854.400134205, 0.673880395 }, //
          {0, 0, 0} //
  };

  static const double b[][3] = { //
          {  102.156724,    6283.075849991, 4.249032005 }, //
          {    1.706807,   12566.151699983, 4.205904248 }, //
          {    0.269668,     213.299095438, 3.400290479 }, //
          {    0.265919,     529.690965095, 5.836047367 }, //
          {    0.210568,      -3.523118349, 6.262738348 }, //
          {    0.077996,    5223.693919802, 4.670344204 }, //
          {    0.059146,      26.298319800, 1.083044735 }, //
          {    0.054764,    1577.343542448, 4.534800170 }, //
          {    0.034420,    -398.149003408, 5.980077351 }, //
          {    0.033595,    5507.553238667, 5.980162321 }, //
          {    0.032088,   18849.227549974, 4.162913471 }, //
          {    0.029198,    5856.477659115, 0.623811863 }, //
          {    0.027764,     155.420399434, 3.745318113 }, //
          {    0.025190,    5746.271337896, 2.980330535 }, //
          {    0.024976,    5760.498431898, 2.467913690 }, //
          {    0.022997,    -796.298006816, 1.174411803 }, //
          {    0.021774,     206.185548437, 3.854787540 }, //
          {    0.017925,    -775.522611324, 1.092065955 }, //
          {    0.013794,     426.598190876, 2.699831988 }, //
          {    0.013276,    6062.663207553, 5.845801920 }, //
          {    0.012869,    6076.890301554, 5.333425680 }, //
          {    0.012152,    1059.381930189, 6.222874454 }, //
          {    0.011774,   12036.460734888, 2.292832062 }, //
          {    0.011081,      -7.113547001, 5.154724984 }, //
          {    0.010143,    4694.002954708, 4.044013795 }, //
          {    0.010084,     522.577418094, 0.749320262 }, //
          {    0.009357,    5486.777843175, 3.416081409 }, //
          {0, 0, 0} //
  };

  static const double c[][3] = { //
          {    0.370115,       0.000000000, 4.712388980 }, //
          {    4.322990,    6283.075849991, 2.642893748 }, //
          {    0.122605,   12566.151699983, 2.438140634 }, //
          {    0.019476,     213.299095438, 1.642186981 }, //
          {    0.016916,     529.690965095, 4.510959344 }, //
          {    0.013374,      -3.523118349, 1.502210314 }, //
          {0 ,0 , 0} //
  };

  static const double d[3] = { 0.143388, 6283.075849991, 1.131453581 };

  const double t = (jd_tt - NOVAS_JD_J2000) / (10.0 * JULIAN_CENTURY_DAYS);
  const double t2 = t * t;

  double us = 0.0;
  int i;

  if(limit < 0.0)
    limit = 0.0;

  for(i = 0; a[i][0] > limit; i ++)
    us += a[i][0] * sin(a[i][1] * t + a[i][2]);

  limit *= fabs(t);
  for(i = 0; b[i][0] > limit; i ++)
    us += b[i][0] * t * sin(b[i][1] * t + b[i][2]);

  limit *= fabs(t);
  for(i = 0; c[i][0] > limit; i ++)
    us += c[i][0] * t2 * sin(c[i][1] * t + c[i][2]);

  limit *= fabs(t);
  if(d[0] > limit)
    us += d[0] * t2 * t * sin(d[1] * t + d[2]);

  return 1e-6 * us;
}

/**
 * Returns the TDB-TT time difference with flexible precision. This implementation uses the full
 * series expansion by Fairhead &amp; Bretagnon 1990, resulting in an accuracy below 100 ns.
 *
 * REFERENCES:
 * <ol>
 * <li>Fairhead, L., &amp; Bretagnon, P. (1990) A&amp;A, 229, 240</li>
 * </ol>
 *
 * @param jd_tt   [day] Terrestrial Time (TT) based Julian date, but Barycentric Dynamical Time (TDB)
 * @return        [s] TDB - TT time difference.
 *
 * @since 1.4
 * @author Attila Kovacs
 *
 * @sa tt2tdb_fp(), tt2tdb()
 */
double tt2tdb_hp(double jd_tt) {
  return tt2tdb_fp(jd_tt, 0.0);
}

/**
 * Returns the difference between Terrestrial Time (TT) and Universal Coordinated Time (UTC)
 *
 * @param leap_seconds  [s] The current leap seconds (see IERS Bulletins)
 * @return              [s] The TT - UTC time difference
 *
 * @since 1.0
 * @author Attila Kovacs
 *
 * @sa get_ut1_to_tt()
 */
double get_utc_to_tt(int leap_seconds) {
  return leap_seconds + NOVAS_TAI_TO_TT;
}

/**
 * Returns the TT - UT1 time difference given the leap seconds and the actual UT1 - UTC time
 * difference as measured and published by IERS.
 *
 * NOTES:
 * <ol>
 * <li>The current UT1 - UTC time difference, and polar offsets, historical data and near-term
 * projections are published in the
 <a href="https://www.iers.org/IERS/EN/Publications/Bulletins/bulletins.html>IERS Bulletins</a>
 * </li>
 * </ol>
 *
 * @param leap_seconds  [s] Leap seconds at the time of observations
 * @param dut1          [s] UT1 - UTC time difference [-0.5:0.5]
 * @return              [s] The TT - UT1 time difference that is suitable for used with all
 *                      calls in this library that require a <code>ut1_to_tt</code> argument.
 *
 * @since 1.0
 * @author Attila Kovacs
 *
 * @sa get_utc_to_tt(), novas_set_time(), novas_gast(), novas_time_gst(), novas_time_lst()
 */
double get_ut1_to_tt(int leap_seconds, double dut1) {
  return get_utc_to_tt(leap_seconds) + dut1;
}

/**
 * Sets an astronomical time to the fractional Julian Date value, defined in the specified
 * timescale. The time set this way is accurate to a few &mu;s (microseconds) due to the inherent
 * precision of the double-precision argument. For higher precision applications you may use
 * `novas_set_split_time()` instead, which has an inherent accuracy at the picosecond level.
 *
 * @param timescale     The astronomical time scale in which the Julian Date is given
 * @param jd            [day] Julian day value in the specified timescale
 * @param leap          [s] Leap seconds, e.g. as published by IERS Bulletin C.
 * @param dut1          [s] UT1-UTC time difference, e.g. as published in IERS Bulletin A, and
 *                      possibly corrected for diurnal and semi-diurnal variations, e.g.
 *                      via `novas_diurnal_eop()`. If the time offset is defined for a different
 *                      ITRS realization than what is used for the coordinates of an Earth-based
 *                      observer, you can use `novas_itrf_transform_eop()` to make it consistent.
 * @param[out] time     Pointer to the data structure that uniquely defines the astronomical time
 *                      for all applications.
 * @return              0 if successful, or else -1 if there was an error (errno will be set to
 *                      indicate the type of error).
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa novas_set_split_time(), novas_set_unix_time(), novas_set_str_time(),
 *     novas_set_current_time(), novas_get_time(), novas_timescale_for_string(),
 *     novas_diurnal_eop()
 */
int novas_set_time(enum novas_timescale timescale, double jd, int leap, double dut1, novas_timespec *restrict time) {
  prop_error("novas_set_time", novas_set_split_time(timescale, 0, jd, leap, dut1, time), 0);
  return 0;
}

/**
 * Sets astronomical time in a specific timescale using a string specification. It is effectively just
 * a shorthand for using `novas_parse_date()` followed by `novas_set_time()` and the error checking
 * in-between.
 *
 * @param timescale     The astronomical time scale in which the Julian Date is given
 * @param str           The astronomical date specification, possibly including time and timezone,
 *                      in a standard format. The date is assumed to be in the astronomical
 *                      calendar of date, which differs from ISO 8601 timestamps for dates prior
 *                      to the Gregorian calendar reform of 1582 October 15 (otherwise, the two
 *                      are identical). See `novas_parse_date()` for more on acceptable formats.
 * @param leap          [s] Leap seconds, e.g. as published by IERS Bulletin C.
 * @param dut1          [s] UT1-UTC time difference, e.g. as published in IERS Bulletin A, and
 *                      possibly corrected for diurnal and semi-diurnal variations, e.g.
 *                      via `novas_diurnal_eop()`. If the time offset is defined for a different
 *                      ITRS realization than what is used for the coordinates of an Earth-based
 *                      observer, you can use `novas_itrf_transform_eop()` to make it consistent.
 * @param[out] time     Pointer to the data structure that uniquely defines the astronomical time
 *                      for all applications.
 * @return              0 if successful, or else -1 if there was an error (errno will be set to
 *                      indicate the type of error).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_set_time(), novas_parse_date()
 */
int novas_set_str_time(enum novas_timescale timescale, const char *restrict str, int leap, double dut1, novas_timespec *restrict time) {
  double jd = novas_parse_date(str, NULL);
  if(isnan(jd))
    return novas_trace("novas_set_str_time", -1, 0);

  return novas_set_time(timescale, jd, leap, dut1, time);
}

/**
 * Sets an astronomical time to the split Julian Date value, defined in the specified timescale.
 * The split into the integer and fractional parts can be done in any convenient way. The highest
 * precision is reached if the fractional part is &le; 1 day. In that case, the time may be
 * specified to picosecond accuracy, if needed.
 *
 * The accuracy of Barycentric Time measures (TDB and TCB) relative to other time measures is
 * limited by the precision of `tbd2tt()` implementation, to around 10 &mu;s.
 *
 *
 * REFERENCES:
 * <ol>
 * <li>IAU 1991, RECOMMENDATION III. XXIst General Assembly of the
 * International Astronomical Union. Retrieved 6 June 2019.</li>
 * <li>IAU 2006 resolution 3, see Recommendation and footnotes, note 3.</li>
 * <li>Fairhead, L. &amp; Bretagnon, P. (1990) Astron. &amp; Astrophys. 229, 240.</li>
 * <li>Kaplan, G. (2005), US Naval Observatory Circular 179.</li>
 * <li><a href="https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/FORTRAN/req/time.html#The%20Relationship%20between%20TT%20and%20TDB">
 * https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/FORTRAN/req/time.html</a></li>
 * <li><a href="https://gssc.esa.int/navipedia/index.php/Transformations_between_Time_Systems">
 * https://gssc.esa.int/navipedia/index.php/Transformations_between_Time_Systems</a></li>
 * </ol>
 *
 * @param timescale     The astronomical time scale in which the Julian Date is given
 * @param ijd           [day] integer part of the Julian day in the specified timescale
 * @param fjd           [day] fractional part Julian day value in the specified timescale
 * @param leap          [s] Leap seconds, e.g. as published by IERS Bulletin C.
 * @param dut1          [s] UT1-UTC time difference, e.g. as published in IERS Bulletin A, and
 *                      possibly corrected for diurnal and semi-diurnal variations, e.g.
 *                      via `novas_diurnal_eop()`. If the time offset is defined for a different
 *                      ITRS realization than what is used for the coordinates of an Earth-based
 *                      observer, you can use `novas_itrf_transform_eop()` to make it consistent.
 * @param[out] time     Pointer to the data structure that uniquely defines the astronomical time
 *                      for all applications.
 * @return              0 if successful, or else -1 if there was an error (errno will be set to
 *                      indicate the type of error).
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa novas_set_time(), novas_set_unix_time(), novas_get_split_time(), novas_timescale_for_string(),
 *     novas_diurnal_eop()
 */
int novas_set_split_time(enum novas_timescale timescale, long ijd, double fjd, int leap, double dut1,
        novas_timespec *restrict time) {
  static const char *fn = "novas_set_split_time";

  long ifjd;

  if(!time)
    return novas_error(-1, EINVAL, "novas_set_time", "NULL output time structure");

  time->tt2tdb = NAN;
  time->dut1 = dut1;
  time->ut1_to_tt = leap - dut1 + DTA * DAY;    // TODO check!

  switch(timescale) {
    case NOVAS_TT:
      break;
    case NOVAS_TCB:
      time->tt2tdb = tt2tdb(ijd + fjd);
      fjd -= time->tt2tdb / DAY - TC_TDB0;
      fjd -= TC_LB * ((ijd - TC_T0) + fjd);
      break;
    case NOVAS_TCG:
      fjd -= TC_LG * ((ijd - TC_T0) + fjd);
      break;
    case NOVAS_TDB: {
      time->tt2tdb = tt2tdb(ijd + fjd);
      fjd -= time->tt2tdb / DAY;
      break;
    }
    case NOVAS_TAI:
      fjd += DTA;
      break;
    case NOVAS_GPS:
      fjd += (DTA + GPS2TAI);
      break;
    case NOVAS_UTC:
      fjd += (time->ut1_to_tt + time->dut1) / DAY;
      break;
    case NOVAS_UT1:
      fjd += time->ut1_to_tt / DAY;
      break;
    default:
      return novas_error(-1, EINVAL, fn, "Invalid timescale: %d", timescale);
  }

  ifjd = (long) floor(fjd);

  time->ijd_tt = ijd + ifjd;
  time->fjd_tt = fjd - ifjd;

  if(isnan(time->tt2tdb))
    time->tt2tdb = tt2tdb(time->ijd_tt + time->fjd_tt);

  return 0;
}

/**
 * Increments the astrometric time by a given amount.
 *
 * @param time        Original time specification
 * @param seconds     [s] Seconds to add to the original
 * @param[out] out    New incremented time specification. It may be the same as the input.
 * @return            0 if successful, or else -1 if either the input or the output is NULL
 *                    (errno will be set to EINVAL).
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa novas_set_time(), novas_diff_time()
 */
int novas_offset_time(const novas_timespec *time, double seconds, novas_timespec *out) {
  long dd;

  if(!time || !out)
    return novas_error(-1, EINVAL, "novas_offset_time", "NULL parameter: time=%p, out=%p", time, out);

  if(out != time)
    *out = *time;

  out->fjd_tt += seconds / DAY;
  dd = (long) floor(out->fjd_tt);
  if(dd) {
    out->fjd_tt -= dd;
    out->ijd_tt += dd;
  }

  return 0;
}

/**
 * Returns the fractional Julian date of an astronomical time in the specified timescale. The
 * returned time is accurate to a few &mu;s (microsecond) due to the inherent precision of the
 * double-precision result. For higher precision applications you may use `novas_get_split_time()`
 * instead, which has an inherent accuracy at the picosecond level.
 *
 * @param time        Pointer to the astronomical time specification data structure.
 * @param timescale   The astronomical time scale in which the returned Julian Date is to be
 *                    provided
 * @return            [day] The Julian date in the requested timescale.
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa novas_set_time(), novas_get_split_time()
 */
double novas_get_time(const novas_timespec *restrict time, enum novas_timescale timescale) {
  long ijd;
  double fjd = novas_get_split_time(time, timescale, &ijd);
  if(isnan(fjd))
    return novas_trace_nan("novas_get_time");
  return ijd + fjd;
}

/**
 * Returns the fractional Julian date of an astronomical time in the specified timescale, as an
 * integer and fractional part. The two-component split of the time allows for absolute precisions
 * at the picosecond level, as opposed to `novas_set_time()`, whose precision is limited to a
 * few microseconds typically.
 *
 * The accuracy of Barycentric Time measures (TDB and TCB) relative to other time measures is
 * limited by the precision of the `tbd2tt()` implemenation, to around 10 &mu;s.
 *
 * REFERENCES:
 * <ol>
 * <li>IAU 1991, RECOMMENDATION III. XXIst General Assembly of the
 * International Astronomical Union. Retrieved 6 June 2019.</li>
 * <li>IAU 2006 resolution 3, see Recommendation and footnotes, note 3.</li>
 * <li>Fairhead, L. &amp; Bretagnon, P. (1990) Astron. &amp; Astrophys. 229, 240.</li>
 * <li>Kaplan, G. (2005), US Naval Observatory Circular 179.</li>
 * <li><a href="https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/FORTRAN/req/time.html#The%20Relationship%20between%20TT%20and%20TDB">
 * https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/FORTRAN/req/time.html</a></li>
 * <li><a href="https://gssc.esa.int/navipedia/index.php/Transformations_between_Time_Systems">
 * https://gssc.esa.int/navipedia/index.php/Transformations_between_Time_Systems</a></li>
 * </ol>
 *
 * @param time        Pointer to the astronomical time specification data structure.
 * @param timescale   The astronomical time scale in which the returned Julian Date is to be
 *                    provided
 * @param[out] ijd    [day] The integer part of the Julian date in the requested timescale. It may
 *                    be NULL if not required.
 * @return            [day] The fractional part of the Julian date in the requested timescale or
 *                    NAN is the time argument is NULL (ijd will be set to -1 also).
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa novas_set_split_time(), novas_get_time()
 */
double novas_get_split_time(const novas_timespec *restrict time, enum novas_timescale timescale, long *restrict ijd) {
  static const char *fn = "novas_get_split_time";
  double f;

  if(ijd) *ijd = -1;

  if(!time) {
    novas_set_errno(EINVAL, fn, "NULL input time specification");
    return NAN;
  }

  if(ijd)
    *ijd = time->ijd_tt;

  f = time->fjd_tt;

  switch(timescale) {
    case NOVAS_TT:
      break;
    case NOVAS_TDB:
      f += time->tt2tdb / DAY;
      break;
    case NOVAS_TCB:
      f += time->tt2tdb / DAY - TC_TDB0;
      f += TC_LB * ((time->ijd_tt - TC_T0) + f);
      break;
    case NOVAS_TCG:
      f += TC_LG * ((time->ijd_tt - TC_T0) + f);
      break;
    case NOVAS_TAI:
      f -= DTA;
      break;
    case NOVAS_GPS:
      f -= (DTA + GPS2TAI);
      break;
    case NOVAS_UTC:
      f -= (time->ut1_to_tt + time->dut1) / DAY;
      break;
    case NOVAS_UT1:
      f -= time->ut1_to_tt / DAY;
      break;
    default:
      novas_set_errno(EINVAL, fn, "Invalid timescale: %d", timescale);
      return NAN;
  }

  if(f < 0.0) {
    f += 1.0;
    if(ijd)
      (*ijd)--;
  }
  else if(f > 1.0) {
    f -= 1.0;
    if(ijd)
      (*ijd)++;
  }

  return f;
}

/**
 * Returns the Terrestrial Time (TT) based time difference (t1 - t2) in days between two
 * astronomical time specifications.
 *
 * @param t1    First time
 * @param t2    Second time
 * @return      [s] Precise time difference (t1-t2), or NAN if one of the inputs was NULL (errno
 *              will be set to EINVAL)
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa novas_set_time(), novas_offset_time(), novas_diff_tcb(), novas_diff_tcg()
 */
double novas_diff_time(const novas_timespec *t1, const novas_timespec *t2) {
  if(!t1 || !t2) {
    novas_set_errno(EINVAL, "novas_diff_time", "NULL parameter: t1=%p, t2=%p", t1, t2);
    return NAN;
  }

  return ((t1->ijd_tt - t2->ijd_tt) + (t1->fjd_tt - t2->fjd_tt)) * DAY;
}

/**
 * Returns the Barycentric Coordinate Time (TCB) based time difference (t1 - t2) in days between
 * two astronomical time specifications. TCB progresses slightly faster than time on Earth, at a
 * rate about 1.6&times10<sup>-8</sup> higher, due to the lack of gravitational time dilation by
 * the Earth or Sun.
 *
 * @param t1    First time
 * @param t2    Second time
 * @return      [s] Precise TCB time difference (t1-t2), or NAN if one of the inputs was
 *              NULL (errno will be set to EINVAL)
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa novas_diff_tcg(), novas_diff_time()
 */
double novas_diff_tcb(const novas_timespec *t1, const novas_timespec *t2) {
  double dt = novas_diff_time(t1, t2) * (1.0 + TC_LB);
  if(isnan(dt))
    return novas_trace_nan("novas_diff_tcb");
  return dt;
}

/**
 * Returns the Geocentric Coordinate Time (TCG) based time difference (t1 - t2) in days between
 * two astronomical time specifications. TCG progresses slightly faster than time on Earth, at a
 * rate about 7&times10<sup>-10</sup> higher, due to the lack of gravitational time dilation by
 * Earth. TCG is an appropriate time measure for a spacecraft that is in the proximity of the
 * orbit of Earth, but far enough from Earth such that the relativistic effects of Earth's gravity
 * can be ignored.
 *
 * @param t1    First time
 * @param t2    Second time
 * @return      [s] Precise TCG time difference (t1-t2), or NAN if one of the inputs was
 *              NULL (errno will be set to EINVAL)
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa novas_diff_tcb(), novas_diff_time()
 */
double novas_diff_tcg(const novas_timespec *t1, const novas_timespec *t2) {
  double dt = novas_diff_time(t1, t2) * (1.0 + TC_LG);
  if(isnan(dt))
    return novas_trace_nan("novas_diff_tcg");
  return dt;
}

/**
 * Sets an astronomical time to a UNIX time value. UNIX time is defined as UTC seconds measured
 * since 0 UTC, 1 Jan 1970 (the start of the UNIX era). Specifying time this way supports
 * precisions to the nanoseconds level by construct. Specifying UNIX time in split seconds and
 * nanoseconds is a common way CLIB handles precision time, e.g. with `struct timespec` and
 * functions like `clock_gettime()` (see `time.h`).
 *
 * @param unix_time   [s] UNIX time (UTC) seconds
 * @param nanos       [ns] UTC sub-second component
 * @param leap        [s] Leap seconds, e.g. as published by IERS Bulletin C.
 * @param dut1        [s] UT1-UTC time difference, e.g. as published in IERS Bulletin A, and
 *                    possibly corrected for diurnal and semi-diurnal variations, e.g.
 *                    via `novas_diurnal_eop()`. If the time offset is defined for a different
 *                    ITRS realization than what is used for the coordinates of an Earth-based
 *                    observer, you can use `novas_itrf_transform_eop()` to make it consistent.
 * @param[out] time   Pointer to the data structure that uniquely defines the astronomical time
 *                    for all applications.
 * @return            0 if successful, or else -1 if there was an error (errno will be set to
 *                    indicate the type of error).
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa novas_set_current_time(), novas_set_time(), novas_get_unix_time(), novas_diurnal_eop()
 */
int novas_set_unix_time(time_t unix_time, long nanos, int leap, double dut1, novas_timespec *restrict time) {
  long jd, sojd;

  // UTC based integer JD
  unix_time -= UNIX_J2000;
  jd = IJD_J2000 + unix_time / IDAY;

  // seconds of JD date
  sojd = unix_time % IDAY;
  if(sojd < 0) {
    sojd += IDAY;
    jd--;
  }

  prop_error("novas_set_unix_time", novas_set_split_time(NOVAS_UTC, jd, (sojd + 1e-9 * nanos) / DAY, leap, dut1, time), 0);
  return 0;
}

/**
 * Sets the time eith the UNIX time obtained from the system clock. This is only as precise as the
 * system clock is. You should generally make sure the sytem clock is synchronized to a time reference
 * e.g. via ntp, preferably to a local time reference.
 *
 * @param leap        [s] Leap seconds, e.g. as published by IERS Bulletin C.
 * @param dut1        [s] UT1-UTC time difference, e.g. as published in IERS Bulletin A, and
 *                    possibly corrected for diurnal and semi-diurnal variations, e.g.
 *                    via `novas_diurnal_eop()`. If the time offset is defined for a different
 *                    ITRS realization than what is used for the coordinates of an Earth-based
 *                    observer, you can use `novas_itrf_transform_eop()` to make it consistent.
 * @param[out] time   Pointer to the data structure that uniquely defines the astronomical time
 *                    for all applications.
 * @return            0 if successful, or else -1 if there was an error (errno will be set to
 *                    indicate the type of error).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @see novas_set_unix_time()
 */
// int novas_set_current_time(int leap, double dut1, novas_timespec *restrict time) {
//   struct timespec t = {};
//   clock_gettime(CLOCK_REALTIME, &t);
//   prop_error("novas_set_current_time", novas_set_unix_time(t.tv_sec, t.tv_nsec, leap, dut1, time), 0);
//   return 0;
// }

/**
 * Returns the UNIX time for an astronomical time instant.
 *
 * @param time        Pointer to the astronomical time specification data structure.
 * @param[out] nanos  [ns] UTC sub-second component. It may be NULL if not required.
 * @return            [s] The integer UNIX time, or -1 if the input time is NULL.
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa novas_set_unix_time(), novas_get_time()
 */
time_t novas_get_unix_time(const novas_timespec *restrict time, long *restrict nanos) {
  long ijd, isod;
  double sod;
  time_t seconds;

  sod = novas_get_split_time(time, NOVAS_UTC, &ijd) * DAY;
  if(isnan(sod)) {
    static const char *fn = "novas_get_unix_time";
    if(nanos) *nanos = novas_trace_nan(fn);
    return novas_trace(fn, -1, 0);
  }

  isod = (long) floor(sod);
  seconds = UNIX_J2000 + (ijd - IJD_J2000) * IDAY + isod;

  if(nanos) {
    *nanos = (long) floor(1e9 * (sod - isod) + 0.5);
    if(*nanos == E9) {
      seconds++;
      *nanos = 0;
    }
  }

  return seconds;
}

static int skip_white(const char *str, char **tail) {
  char *next = (char *) str;

  // Consume trailing 'white' spaces / punctuation
  for(; *next; next++)
    if(!isspace(*next) && *next != '_')
      break;

  *tail = next;
  return 0;
}

static int parse_zone(const char *str, char **tail) {
  char *next = (char *) str;

  *tail = next;

  if(*str == '+' || *str == '-') {
    static const char *fn = "parse_zone";

    // zone in {+|-}HH[:[MM]] format...
    int H = 0, M = 0;
    int sign = *(next++) == '-' ? -1 : 1;
    int colon = 0;

    if(isdigit(next[0]) && isdigit(next[1])) {
      H = 10 * (next[0] - '0') + (next[1] - '0');
      if(H >= 24)
        return novas_error(-1, EINVAL, fn, "invalid zone hours: %d, expected [0-23]", H);
      next += 2;
    }
    else
      return novas_error(-1, EINVAL, fn, "invalid time zone specification");

    if(*next == ':') {
      next++;
      colon = 1;
    }

    if(isdigit(next[0])) {
      if(!isdigit(next[1]))
        return novas_error(-1, EINVAL, fn, "invalid time zone specification");

      M = 10 * (next[0] - '0') + (next[1] - '0');
      if(M >= 60)
        return novas_error(-1, EINVAL, fn, "invalid zone minutes: %d, expected [0-60]", M);
      next += 2;
    }
    else if(colon)
      next--;

    *tail = next;
    return sign * (H * 3600 + M * 60); // zone time to UTC...
  }

  if(*str == 'Z' || *str == 'z')
    *tail = (char *) str + 1;

  return 0;
}

/**
 * Parses a calndar date/time string, expressed in the specified type of calendar, into a Julian
 * day (JD). The date must be composed of a full year (e.g. 2025), a month (numerical or name or
 * 3-letter abbreviation, e.g. "01", "1", "January", or "Jan"), and a day (e.g. "08" or "8"). The
 * components may be separated by dash `-`, underscore `_`, dot `.`,  slash '/', or spaces/tabs,
 * or any combination thereof. The components will be parsed in the specified order.
 *
 * The date may be followed by a time specification in HMS format, separated from the date by the
 * letter `T` or `t`, or spaces, comma `,`, or semicolon `;` or underscore '_', or a combination
 * thereof. Finally, the time may be followed by the letter `Z`, or `z` (for UTC) or else by a
 * {+/-}HH[:[MM]] time zone specification.
 *
 * For example, for `format` NOVAS_YMD, all of the following strings may specify the date:
 *
 * <pre>
 *  2025-01-26
 *  2025 January 26
 *  2025_Jan_26
 *  2025-01-26T19:33:08Z
 *  2025.01.26T19:33:08
 *  2025 1 26 19h33m28.113
 *  2025/1/26 19:33:28+02
 *  2025-01-26T19:33:28-0600
 *  2025 Jan 26 19:33:28+05:30
 * </pre>
 *
 * are all valid dates that can be parsed.
 *
 * If your date format cannot be parsed with this function, you may parse it with your own
 * function into year, month, day, and decimal hour-of-day components, and use julian_date() with
 * those.
 *
 * NOTES:
 * <ol>
 *  <li>B.C. dates are indicated with years &lt;=0 according to the astronomical
 * and ISO 8601 convention, i.e., X B.C. as (1-X), so 45 B.C. as -44.</li>
 * </oL>
 *
 * @param calendar    The type of calendar to use: NOVAS_ASTRONOMICAL_CALENDAR,
 *                    NOVAS_GREGORIAN_CALENDAR, or NOVAS_ROMAN_CALENDAR.
 * @param format      Expected order of date components: NOVAS_YMD, NOVAS_DMY, or NOVAS_MDY.
 * @param date        The date specification, possibly including time and timezone, in the
 *                    specified standard format.
 * @param[out] tail   (optional) If not NULL it will be set to the next character in the string
 *                    after the parsed time.
 *
 * @return            [day] The Julian Day corresponding to the string date/time specification or
 *                    NAN if the string is NULL or if it does not specify a date/time in the
 *                    expected format.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_parse_date(), novas_parse_iso_date(), novas_timescale_for_string(), novas_iso_timestamp()
 *     novas_jd_from_date()
 */
double novas_parse_date_format(enum novas_calendar_type calendar, enum novas_date_format format, const char *restrict date,
        char **restrict tail) {
  static const char *fn = "novas_parse_date";
  static const char md[13] = { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  int y = 0, m = 0, d = 0, n = 0, N = 0;
  double h = 0.0;
  char month[10] = {'\0'}, *next = (char *) date;

  if(tail)
    *tail = (char *) date;

  if(!date) {
    novas_set_errno(EINVAL, fn, "input string is NULL");
    return NAN;
  }
  if(!date[0]) {
    novas_set_errno(EINVAL, fn, "input string is empty");
    return NAN;
  }

  switch(format) {
    case NOVAS_YMD:
      N = sscanf(date, "%d" DATE_SEP MONTH_SPEC DATE_SEP "%d%n", &y, month, &d, &n);
      break;
    case NOVAS_DMY:
      N = sscanf(date, "%d" DATE_SEP MONTH_SPEC DATE_SEP "%d%n", &d, month, &y, &n);
      break;
    case NOVAS_MDY:
      N = sscanf(date, MONTH_SPEC DATE_SEP "%d" DATE_SEP "%d%n", month, &d, &y, &n);
      break;
    default:
      novas_set_errno(EINVAL, fn, "invalid date format: %d", format);
      return NAN;
  }

  if(N < 3) {
    novas_set_errno(EINVAL, fn, "invalid date: '%s'", date);
    return NAN;
  }

  if(sscanf(month, "%d", &m) == 1) {
    // Month as integer, check if in expected range
    if(m < 1 || m > 12) {
      novas_set_errno(EINVAL, fn, "invalid month: got %d, expected 1-12", m);
      return NAN;
    }
  }
  else {
    // Perhaps month as string...
    for(m = 1; m <= 12; m++) {
      static const char *monNames[13] = { NULL, "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

      if(strcasecmp(monNames[m], month) == 0)
        break;      // match full month name
      if(strncasecmp(monNames[m], month, 3) == 0)
        break;      // match abbreviated month name
    }
    if(m > 12) {
      // No match to month names...
      novas_set_errno(EINVAL, fn, "invalid month: %s", month);
      return NAN;
    }
  }

  // Check that day is valid in principle (for leap years)
  if(d < 1 || d > md[m]) {
    novas_set_errno(EINVAL, fn, "invalid day-of-month: got %d, expected 1-%d", d, md[m]);
    return NAN;
  }

  if(tail)
    *tail += n;

  skip_white(&date[n], &next);

  if(*next) {
    char *from = next;
    enum novas_debug_mode saved = novas_get_debug_mode();

    // Check if 'T' is used to separate time component, as in ISO timestamps.
    if(*next == 'T' || *next == 't')
      next++;

    // suppress debug messages while we parse time...
    novas_debug(NOVAS_DEBUG_OFF);

    // Try parse time
    h = novas_parse_hms(next, &next);

    // Restore prior debug state...
    errno = 0;
    novas_debug(saved);

    if(!isnan(h)) {
      int ds = parse_zone(next, &next);
      if(errno)
        return novas_trace_nan(fn);
      h -= ds / 3600.0;
    }
    else if(tail) {
      h = 0.0;
      next = from; // Time parsing unsuccessful, no extra characters consumed.
    }

    if(tail)
      *tail = next;
  }

  return novas_jd_from_date(calendar, y, m, d, h);
}

/**
 * Parses an astronomical date/time string into a Julian date specification.
 *
 * The date must be YMD-type with full year, followed the month (numerical or name or 3-letter
 * abbreviation), and the day. The components may be separated by dash `-`, underscore `_`, dot
 * `.`,  slash '/', or spaces/tabs, or any combination thereof. The date may be followed by a time
 * specification in HMS format, separated from the date by the letter `T` or `t`, or spaces, comma
 * `,`, or semicolon `;`, or underscore `_` or a combination thereof. Finally, the time may be
 * followed by the letter `Z`, or `z` (for UTC) or else {+/-}HH[:[MM]] time zone specification.
 *
 * For example:
 *
 * <pre>
 *  2025-01-26
 *  2025 January 26
 *  2025_Jan_26
 *  2025-01-26T19:33:08Z
 *  2025.01.26T19:33:08
 *  2025 1 26 19h33m28.113
 *  2025/1/26 19:33:28+02
 *  2025-01-26T19:33:28-0600
 *  2025 Jan 26 19:33:28+05:30
 * </pre>
 *
 * are all valid dates that can be parsed.
 *
 * NOTES:
 * <ol>
 * <li>This function assumes Gregorian dates after their introduction on 1582 October 15, and
 * Julian/Roman dates before that, as was the convention of the time. I.e., the day before of the
 * introduction of the Gregorian calendar reform is 1582 October 4. I.e., you should not use
 * this function with ISO 8601 timestamps containing dates prior to 1582 October 15 (for such
 * date you may use `novas_parse_iso_date()` instead).</li>
 *
 * <li>B.C. dates are indicated with years &lt;=0 according to the astronomical
 * and ISO 8601 convention, i.e., X B.C. as (1-X), so 45 B.C. as -44.</li>
 * </oL>
 *
 * @param date        The astronomical date specification, possibly including time and timezone,
 *                    in a standard format. The date is assumed to be in the astronomical calendar
 *                    of date, which differs from ISO 8601 timestamps for dates prior to the
 *                    Gregorian calendar reform of 1582 October 15 (otherwise, the two are
 *                    identical).
 * @param[out] tail   (optional) If not NULL it will be set to the next character in the string
 *                    after the parsed time.
 *
 * @return            [day] The Julian Day corresponding to the string date/time specification or
 *                    NAN if the string is NULL or if it does not specify a date/time in the
 *                    expected format.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_parse_iso_date(), novas_parse_date_format(), novas_date(), novas_date_scale(),
 *     novas_timescale_for_string(), novas_iso_timestamp(), novas_timestamp()
 */
double novas_parse_date(const char *restrict date, char **restrict tail) {
  double jd = novas_parse_date_format(NOVAS_ASTRONOMICAL_CALENDAR, NOVAS_YMD, date, tail);
  if(isnan(jd))
    return novas_trace_nan("novas_parse_date");
  return jd;
}

/**
 * Parses an ISO 8601 timestamp, converting it to a Julian day. It is equivalent to
 * `novas_parse_date()` for dates after the Gregorian calendar reform of 1582. For earlier dates,
 * ISO timestamps continue to assume the Gregorian calendar (i.e. proleptic Gregorian dates),
 * whereas `novas_parse_timestamp()` will assume Roman/Julian dates, which were conventionally
 * used before the calendar reform.
 *
 * NOTES:
 * <ol>
 * <li>B.C. dates are indicated with years &lt;=0 according to the astronomical
 * and ISO 8601 convention, i.e., X B.C. as (1-X), so 45 B.C. as -44.</li>
 * </oL>
 *
 * @param date        The ISO 8601 date specification, possibly including time and timezone, in a
 *                    standard format.
 * @param[out] tail   (optional) If not NULL it will be set to the next character in the string
 *                    after the parsed time.
 *
 * @return            [day] The Julian Day corresponding to the string date/time specification or
 *                    NAN if the string is NULL or if it does not specify a date/time in the
 *                    expected format.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_iso_timestamp(), novas_parse_date()
 */
double novas_parse_iso_date(const char *restrict date, char **restrict tail) {
  double jd = novas_parse_date_format(NOVAS_GREGORIAN_CALENDAR, NOVAS_YMD, date, tail);
  if(isnan(jd))
    return novas_trace_nan("novas_parse_iso_date");
  return jd;
}

/**
 * Returns a Julian date (in non-specific timescale) corresponding the specified input string date
 * / time. E.g. for "2025-02-28T09:41:12.041+0200", with some flexibility on how the date is
 * represented as long as it's YMD date followed by HMS time. For other date formats (MDY or DMY)
 * you can use `novas_parse_date_format()` instead.
 *
 * @param date  The date specification, possibly including time and timezone, in a standard
 *              format. See novas_parse_date() on more information on acceptable date/time
 *              formats.
 * @return      [day] The Julian Day corresponding to the string date/time specification or
 *              NAN if the string is NULL or if it does not specify a date/time in the
 *              expected format.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_date_scale(), novas_parse_date(), novas_parse_date_format(), novas_iso_timestamp()
 */
double novas_date(const char *restrict date) {
  double jd = novas_parse_date(date, NULL);
  if(isnan(jd))
    return novas_trace_nan("novas_date");
  return jd;
}

/**
 * Returns a Julian date and the timescale corresponding the specified input string date/time and
 * timescale marker. E.g. for "2025-02-28T09:41:12.041+0200 TAI", with some flexibility on how the
 * date is represented as long as it's YMD date followed by HMS time. For other date formats (MDY
 * or DMY) you can use `novas_parse_date_format()` instead.
 *
 * @param date          The date specification, possibly including time and timezone, in a
 *                      standard format. See novas_parse_date() on more information on
 *                      acceptable date/time formats.
 * @param[out] scale    The timescale constant, or else -1 if the string could not be parsed
 *                      into a date and timescale. If the string is a bare timestamp without a
 *                      hint of a timescale marker, then NOVAS_UTC will be assumed.
 * @return      [day] The Julian Day corresponding to the string date/time specification or
 *              NAN if the string is NULL or if it does not specify a date/time in the
 *              expected format.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_date(), novas_timestamp()
 */
double novas_date_scale(const char *restrict date, enum novas_timescale *restrict scale) {
  const char *fn = "novas_date_scale";

  char *tail = (char *) date;
  double jd = novas_parse_date(date, &tail);

  if(!scale) {
    novas_set_errno(EINVAL, fn, "output scale is NULL");
    return NAN;
  }

  *scale = -1;

  if(isnan(jd))
    return novas_trace_nan(fn);

  *scale = novas_parse_timescale(tail, &tail);

  return jd;
}

static int timestamp(long ijd, double fjd, enum novas_calendar_type cal, char *buf) {
  long dd, ms;
  int y = 0, M = 0, d = 0, h, m, s;

  // fjd -> [-0.5:0.5) range
  dd = (long) floor(fjd + 0.5);
  ijd += dd;
  fjd -= dd;

  // Round to nearest ms.
  ms = (long) floor((fjd + 0.5) * DAY_MILLIS + 0.5);
  if(ms >= DAY_MILLIS) {
    ms -= DAY_MILLIS;     // rounding to 0h next day...
    ijd++;
  }

  // Date at 12pm of the same day
  novas_jd_to_date(ijd, cal, &y, &M, &d, NULL);

  // Time breakdown
  h = (int) (ms / HOUR_MILLIS);
  ms -= HOUR_MILLIS * h;

  m = (int) (ms / MIN_MILLIS);
  ms -= MIN_MILLIS * m;

  s = (int) (ms / 1000L);
  ms -= 1000L * s;

  return sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02d.%03d", y, M, d, h, m, s, (int) ms);
}

/**
 * Prints a UTC-based ISO timestamp to millisecond precision to the specified string buffer. E.g.:
 *
 * <pre>
 *  2025-01-26T21:32:49.701Z
 * </pre>
 *
 * NOTES:
 * <ol>
 * <li>As per the ISO 8601 specification, the timestamp uses the Gregorian date, even for dates
 * prior to the Gregorian calendar reform of 15 October 1582 (i.e. proleptic Gregorian dates).</li>
 *
 * <li>B.C. dates are indicated with years &lt;=0 according to the astronomical
 * and ISO 8601 convention, i.e., X B.C. as (1-X), so 45 B.C. as -44.</li>
 * </ol>
 *
 * @param time      Pointer to the astronomical time specification data structure.
 * @param[out] dst  Output string buffer. At least 25 bytes are required for a complete timestamp
 *                  with termination.
 * @param maxlen    The maximum number of characters that can be printed into the output buffer,
 *                  including the string termination. If the full ISO timestamp is longer than
 *                  `maxlen`, then it will be truncated to fit in the allotted space, including a
 *                  termination character.
 * @return          the number of characters printed into the string buffer, not including the
 *                  termination. As such it is at most `maxlen - 1`.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_timestamp(), novas_parse_iso_date()
 */
int novas_iso_timestamp(const novas_timespec *restrict time, char *restrict dst, int maxlen) {
  static const char *fn = "novas_iso_timestamp";

  char buf[40];
  long ijd = 0;
  double fjd;
  int l;

  if(!dst)
    return novas_error(-1, EINVAL, fn, "output buffer is NULL");

  if(maxlen < 1)
    return novas_error(-1, EINVAL, fn, "invalid maxlen: %d", maxlen);

  *dst = '\0';

  if(!time)
    return novas_error(-1, EINVAL, fn, "input time is NULL");

  fjd = novas_get_split_time(time, NOVAS_UTC, &ijd);
  l = timestamp(ijd, fjd, NOVAS_GREGORIAN_CALENDAR, buf);

  // Add 'Z' to indicate UTC time zone.
  buf[l++] = 'Z';
  buf[l] = '\0';

  if(l >= maxlen)
    l = maxlen - 1;

  strncpy(dst, buf, l);
  dst[l] = '\0';

  return l;
}

/**
 * Prints an astronomical timestamp to millisecond precision in the specified timescale to the
 * specified string buffer. It differs from ISO 8601 timestamps for dates prior to the Gregorian
 * calendar reform of 1582 October 15 (otherwise two are identical). E.g.:
 *
 * <pre>
 *  2025-01-26T21:32:49.701 TAI
 * </pre>
 *
 * NOTES:
 * <ol>
 * <li>The timestamp uses the astronomical date. That is Gregorian dates after the Gregorian
 * calendar reform of 15 October 1582, and Julian/Roman dates prior to that. This is in contrast
 * to ISO 8601 timestamps, which use Gregorian dates even for dates the precede the calendar
 * reform that introduced them.</li>
 *
 * <li>B.C. dates are indicated with years &lt;=0 according to the astronomical and ISO 8601
 * convention, i.e., X B.C. as (1-X), so 45 B.C. as -44.</li>
 * </ol>
 *
 * @param time      Pointer to the astronomical time specification data structure.
 * @param scale     The timescale to use.
 * @param[out] dst  Output string buffer. At least 28 bytes are required for a complete timestamp
 *                  with termination.
 * @param maxlen    The maximum number of characters that can be printed into the output buffer,
 *                  including the string termination. If the full ISO timestamp is longer than
 *                  `maxlen`, then it will be truncated to fit in the allotted space, including a
 *                  termination character.
 * @return          the number of characters printed into the string buffer, not including the
 *                  termination. As such it is at most `maxlen - 1`.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_iso_timestamp(), novas_parse_date()
 */
int novas_timestamp(const novas_timespec *restrict time, enum novas_timescale scale, char *restrict dst, int maxlen) {
  static const char *fn = "novas_timestamp_scale";

  char buf[40];
  long ijd;
  double fjd;
  int n;

  if(!dst)
    return novas_error(-1, EINVAL, fn, "output buffer is NULL");

  if(maxlen < 1)
    return novas_error(-1, EINVAL, fn, "invalid maxlen: %d", maxlen);

  *dst = '\0';

  if(!time)
    return novas_error(-1, EINVAL, fn, "input time is NULL");

  fjd = novas_get_split_time(time, scale, &ijd);
  n = timestamp(ijd, fjd, NOVAS_ASTRONOMICAL_CALENDAR, buf);

  buf[n++] = ' ';

  n += novas_print_timescale(scale, &buf[n]);

  if(n >= maxlen)
    n = maxlen - 1;

  memcpy(dst, buf, n);
  dst[n] = '\0';

  return n;
}

/**
 * Prints the standard string representation of the timescale to the specified buffer. The string
 * is terminated after. E.g. "UTC", or "TAI". It will print dates in the Gregorian calendar, which
 * was introduced in was introduced on 15 October 1582 only. Thus the
 *
 * @param scale   The timescale
 * @param buf     String in which to print. It should have at least 4-bytes of available storage.
 * @return        the number of characters printed, not including termination, or else -1 if the
 *                timescale is invalid or the output buffer is NULL.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_timestamp(), novas_timescale_for_string()
 */
int novas_print_timescale(enum novas_timescale scale, char *restrict buf) {
  static const char *fn = "novas_print_timescale";

  if(!buf)
    return novas_error(-1, EINVAL, fn, "output buffer is NULL");

  switch(scale) {
    case NOVAS_UT1:
      return sprintf(buf, "UT1");
    case NOVAS_UTC:
      return sprintf(buf, "UTC");
    case NOVAS_GPS:
      return sprintf(buf, "GPS");
    case NOVAS_TAI:
      return sprintf(buf, "TAI");
    case NOVAS_TT:
      return sprintf(buf, "TT");
    case NOVAS_TCG:
      return sprintf(buf, "TCG");
    case NOVAS_TCB:
      return sprintf(buf, "TCB");
    case NOVAS_TDB:
      return sprintf(buf, "TDB");
  }

  *buf = '\0';

  return novas_error(-1, EINVAL, fn, "invalid timescale: %d", scale);
}

/**
 * Returns the timescale constant for a string that denotes the timescale in with a standard
 * abbreviation (case insensitive). The following values are recognised: "UTC", "UT", "UT0",
 * "UT1", "GMT", "TAI", "GPS", "TT", "ET", "TCG", "TCB", and "TDB".
 *
 * @param str     String specifying an astronomical timescale
 * @return        The SuperNOVAS timescale constant (&lt;=0), or else -1 if the string was NULL,
 *                empty, or could not be matched to a timescale value (errno will be set to EINVAL
 *                also).
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_parse_timescale(), novas_set_str_time(), novas_print_timescale()
 */
enum novas_timescale novas_timescale_for_string(const char *restrict str) {
  static const char *fn = "novas_str_timescale";

  if(!str)
    return novas_error(-1, EINVAL, fn, "input string is NULL");

  if(!str[0])
    return novas_error(-1, EINVAL, fn, "input string is empty");

  if(strcasecmp("UTC", str) == 0 || strcasecmp("UT", str) == 0 || strcasecmp("UT0", str) == 0 || strcasecmp("GMT", str) == 0)
    return NOVAS_UTC;

  if(strcasecmp("UT1", str) == 0)
    return NOVAS_UT1;

  if(strcasecmp("TAI", str) == 0)
    return NOVAS_TAI;

  if(strcasecmp("GPS", str) == 0)
    return NOVAS_GPS;

  if(strcasecmp("TT", str) == 0 || strcasecmp("ET", str) == 0)
    return NOVAS_TT;

  if(strcasecmp("TCG", str) == 0)
    return NOVAS_TCG;

  if(strcasecmp("TCB", str) == 0)
    return NOVAS_TCB;

  if(strcasecmp("TDB", str) == 0)
    return NOVAS_TDB;

  return novas_error(-1, EINVAL, fn, "unknown timescale: %s", str);
}

/**
 * Parses the timescale from a string containing a standard abbreviation (case insensitive), and
 * returns the updated parse position after the timescale specification (if any). The following
 * timescale values are recognised: "UTC", "UT", "UT0", "UT1", "GMT", "TAI", "GPS", "TT", "ET",
 * "TCG", "TCB", "TDB".
 *
 * @param str         String specifying an astronomical timescale. Leading white spaces will be
                      skipped over.
 * @param[out] tail   (optional) If not NULL it will be set to the next character in the string
 *                    after the parsed timescale specification.
 *
 * @return            The SuperNOVAS timescale constant (&lt;=0), or else -1 if the string was
 *                    NULL, empty, or could not be matched to a timescale value (errno will be set
 *                    to EINVAL also).
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_timescale_for_string(), novas_set_str_time(), novas_print_timescale()
 */
enum novas_timescale novas_parse_timescale(const char *restrict str, char **restrict tail) {
  static const char *fn = "novas_parse_timescale";

  enum novas_timescale scale = NOVAS_UTC;
  char s[4] = {'\0'};
  int n = 0;

  if(tail)
    *tail = (char *) str;

  if(!str)
    return novas_error(-1, EINVAL, fn, "input string is NULL");

  if(sscanf(str, "%3s%n", s, &n) == 1) {
    scale = novas_timescale_for_string(s);
    if(scale < 0)
      return novas_trace(fn, scale, 0);
  }

  if(tail)
    *tail += n;

  return scale;
}

/**
 * Returns the Greenwich (apparent) Sidereal Time (GST/GaST) for a given astronomical time
 * specification. If you need mean sidereal time (GMST), you should use sidereal_time() instead.
 *
 * @param time      The astronomoical time specification.
 * @param accuracy  NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1).
 * @return          [h] The Greenwich apparent Sidereal Time (GST / GaST) in the [0:24) hour
 *                  range, or else NAN if there was an error (`errno` will indicate the type of
 *                  error).
 *
 * @since 1.4
 * @author Attila Kovacs
 *
 * @sa novas_time_lst(), novas_set_time()
 */
double novas_time_gst(const novas_timespec *restrict time, enum novas_accuracy accuracy) {
  double jd_ut1 = novas_get_time(time, NOVAS_UT1);

  if(isnan(jd_ut1))
    return novas_trace_nan("novas_time_gst");

  return novas_gast(jd_ut1, time->ut1_to_tt, accuracy);
}

/**
 * Returns the Local (apparent) Sidereal Time (LST/LaST) for a given astronomical time
 * specification and observer location.
 *
 * @param time      The astronomoical time specification.
 * @param lon       [deg] The geodetic longitude of the observer.
 * @param accuracy  NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1).
 * @return          [h] The Local apparent Sidereal Time (LST / LaST) in the [0:24) hour range, or
 *                  else NAN if there was an error (`errno` will indicate the type of error).
 *
 * @since 1.4
 * @author Attila Kovacs
 *
 * @sa novas_frame_lst(), novas_set_time()
 */
double novas_time_lst(const novas_timespec *restrict time, double lon, enum novas_accuracy accuracy) {
  double st = novas_time_gst(time, accuracy);
  if(isnan(st))
    return novas_trace_nan("novas_time_lst");
  st = remainder(st + lon / 15.0, DAY_HOURS);
  return st < 0.0 ? st + DAY_HOURS : st;
}

/**
 * Returns the Newtonian Solar-system gravitational potential due to the major planets, the Sun,
 * and Moon. If the position is inside one of the bodies (i.e., its distance from the body's
 * center is less than 90% of the body's mean radius), then the potential due to that body is not
 * included in the calculation. For example, if the position is at or near the geocenter (well
 * below the Earth's surface), then the calculation will not include Earth potential.
 *
 * In reduced accuracy mode, the result may be approximate, as only the bodies with known
 * positions are included. The gravitational potential due to the Sun and Earth are always
 * included.
 *
 * @param frame   The observing frame, defining the observer position as well as the positions
 *                of the major solar-system bodies at the time of observation.
 * @param pos     [AU] The observer's position. If it's near the center of one of the gravitating
 *                bodies, that body's potential is excluded from the calculation.
 * @return        The dimensionless Newtonian Solar-system potential, or NAN (errno set to EAGAIN)
 *                if the frame is configured for full accuracy but not all planet positions are
 *                available.
 */
static double solar_system_potential(const novas_frame *frame, const double *pos) {
  double V = 0.0;
  int i;

  // Calculate the Newtonian gravitational potential of the major planets, Sun and the Moon
  for(i = NOVAS_MERCURY; i <= NOVAS_MOON; i++) {
    double d = 0.0;

    if(frame->planets.mask & (1 << i)) {
      // Use the apparent positions calculated for gravitational bending. In low
      // accuracy mode, this may only contain a position for the Sun...
      int k;
      for(k = 3; --k >= 0; ) {
        double x = frame->planets.pos[i][k] + frame->obs_pos[k] - pos[k];
        d += x * x;
      }
      d = sqrt(d) * AU;
    }
    else if(i == NOVAS_EARTH) {
      // We might not have an ephemeris position for Earth above, but we always have
      // an Earth-position (possibly less accurate) as part of the frame itself...
      d = novas_vdist(pos, frame->earth_pos) * AU;
    }
    else if(frame->accuracy != NOVAS_REDUCED_ACCURACY) {
      novas_set_errno(EAGAIN, "solar_system_potential", "Missing position data for planet %d", i);
      return NAN;
    }

    // Skip object if we are inside it (less than 90% radius from the center).
    if(d < 0.9 * R[i])
      continue;

    // Newtonian potential V = -GM / r
    V -= NOVAS_G_SUN / (d * iM[i]);
  }

  return V / (C * C);
}

/**
 * Returns the first derivative of the Solar-system gravitational potential die to the major
 * planets, the Sun, and the Moon.
 *
 * @param frame     The observing frame, defining the observer position as well as the positions
 *                  of the major solar-system bodies at the time of observation.
 * @param pos       [AU] The BCRS position of at which to estimate the tidal potential.
 * @param[out] V1   [1/AU] The _xyz_ gradient of the dimensionless potential in rectangular
 *                  equatorial coordinates.
 */
static void solar_system_tidal_potential(const novas_frame *frame, const double *pos, double *V1) {
  const double eps = 1e-6;  // [AU] ~ 150 km...
  double x[3];
  int i;

  memcpy(x, pos, sizeof(x));
  memset(V1, 0.0, sizeof(x));

  for(i = 0; i < 3; i++) {
    x[i] += eps;
    V1[i] += solar_system_potential(frame, x);
    x[i] = pos[i] - eps;
    V1[i] -= solar_system_potential(frame, x);
    x[i] = pos[i];
    V1[i] /= (2.0 * eps);
  }
}

/**
 * Returns the dimensionless kinetic potential term for a moving observer.
 *
 * @param b     &beta; i.e., _v_ / _c_.
 * @return      The dimensionless kinetic potential for the rate at which time advances for the
 *              observer.
 */
static double kinetic_potential(double b) {
  double ig;

  if(b < 1e-6)
    // IERS Conventions, Chapter 10, Eq. 10.6 -- non-relativistic.
    return -0.5 * b * b;

  ig = sqrt(1.0 - b * b);

  // IERS Conventions, Chapter 10, Eq. 10.6 -- relativistic.
  // dtau / dTCG = (1 - dV/c2) / gamma - 1 -->
  return (ig - 1.0);
}

/**
 * Returns the incremental rate at which the observer's clock (i.e. proper time &tau;) ticks
 * faster than a TCG clock. I.e., it returns _D_, which is defined by:
 *
 * d&tau<sub>obs</sub>; / dTCG = (1 + _D_)
 *
 * @param frame         The observing frame, defining the observer position as well as the
 *                      positions of the major solar-system bodies at the time of observation.
 * @return              _D_.
 */
static double clock_skew_near_earth(const novas_frame *frame) {
  double dV;
  double vo[3], vr, z, b;
  int i;

  dV = solar_system_potential(frame, frame->obs_pos) - solar_system_potential(frame, frame->earth_pos);

  // observer velocity vs reference
  for(i = 3; --i >= 0; )
    vo[i] = novas_add_vel(frame->obs_vel[i], -frame->earth_vel[i]);

  vr = novas_vlen(vo) * (AU / DAY); // geocentric motion
  if(vr >= C)
    return -1.0;

  z = novas_v2z(vr / KMS) / (1.0 + TC_LG); // Reference velocities from TT/TDB to TCG
  b = novas_z2v(z) * KMS / C;

  return kinetic_potential(b) + sqrt(1.0 - b * b) * dV;
}

/**
 * Returns the incremental rate at which an Earth-bound observer's clock (i.e. proper time &tau;)
 * ticks faster due to the local tidal potential around Earth (mainly due to the Sun and Moon).
 * I.e., it returns _D_, which is defined by:
 *
 * d&tau<sub>tidal</sub>; / d&tau<sub>obs</sub>; = (1 + _D_)
 *
 * @param frame         The observing frame, defining the observer position as well as the
 *                      positions of the major solar-system bodies at the time of observation.
 * @return              _D_.
 */
static double tidal_clock_skew(const novas_frame *frame) {
  enum novas_observer_place loc;
  double dV = 0.0, VE1[3] = {0.0};
  int i;

  loc = frame->observer.where;
  switch(loc) {
    case NOVAS_OBSERVER_ON_EARTH:
    case NOVAS_AIRBORNE_OBSERVER:
    case NOVAS_OBSERVER_IN_EARTH_ORBIT:
      solar_system_tidal_potential(frame, frame->earth_pos, VE1);

      for(i = 3; --i >= 0; )
        dV += (frame->obs_pos[i] - frame->earth_pos[i]) * VE1[i];

      break;

    default:
      break;
  }

  return dV;
}

/**
 * Returns the instantaneous incremental rate at which the observer's clock (i.e. proper time
 * &tau;) ticks faster than a clock in the specified timescale. I.e., it returns _D_, which is
 * defined by:
 *
 * d&tau<sub>obs</sub>; / dt<sub>timescale</sub> = (1 + _D_)
 *
 * The instantaneous difference in clock rate includes tiny diurnal or orbital variationd for
 * Earth-bound observers as the they cycle through the tidal potential around the geocenter
 * (mainly due to the Sun and Moon). For a closer match to Earth-based timescales (TCG, TT, TAI,
 * GPS, or UTC) you may want to exclude the periodic tidal effects and calculate the averaged
 * observer clock rate over the geocentric cycle (see Eqs. 10.6 and 10.8 of the IERS Conventions
 * 2010), which is provided by `novas_mean_clock_skew()` instead.
 *
 * For reduced accuracy frames, the result will be approximate, because the gravitational effect
 * of the Sun and Earth alone may be accounted for.
 *
 * NOTES:
 * <ol>
 * <li>Based on the IERS Conventions 2010, Chapter 10, Eqa. 10.6 / 10.8 but also including the
 * near-Earth tidal effects, and modified for relativistic observer motion.</li>
 * <li>The potential for an observer inside 0.9 planet radii of a major Solar-system body's center
 * will not include the term for that body in the calculation.</li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 * <li>IERS Conventions 2010, Chapter 10, see at https://iers-conventions.obspm.fr/content/chapter10/tn36_c10.pdf</li>
 * </ol>
 *
 * @param frame       The observing frame, defining the observer position as well as the positions
 *                    of the major solar-system bodies at the time of observation.
 * @param timescale   Reference timescale for the comparison. All timescales except `NOVAS_UT1`
 *                    are supported. (UT1 advances at an irregular rate).
 * @return            The incremental rate at which the observer's proper time clock ticks faster
 *                    than the specified timescale, or else NAN if the input frame is NULL or
 *                    uninitialized, or if the timescale is not supported (errno set to EINVAL),
 *                    or if the frame is configured for full accuracy but it does not have
 *                    sufficient planet position information to evaluate the local gravitational
 *                    potential with precision (errno set to EAGAIN).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_mean_clock_skew()
 */
double novas_clock_skew(const novas_frame *frame, enum novas_timescale timescale) {
  static const char *fn = "novas_clock_skew";
  double D = 0.0;

  if(!frame) {
    novas_set_errno(EINVAL, fn, "input frame is NULL");
    return NAN;
  }

  if(!novas_frame_is_initialized(frame)) {
    novas_set_errno(EINVAL, fn, "input frame is not initialized");
    return NAN;
  }

  switch(timescale) {
    case NOVAS_TCG: {
      D = clock_skew_near_earth(frame);
      break;
    }

    case NOVAS_TT:
    case NOVAS_TAI:
    case NOVAS_GPS:
    case NOVAS_UTC: {
      D = clock_skew_near_earth(frame);
      // (1 + D) * (1 + D') = 1 + D + (1+ D) D'
      D += TC_LG * (1.0 + D);
      break;
    }

    case NOVAS_TDB: {
      double eps = 0.1; // [day]
      double jd = frame->time.ijd_tt + frame->time.fjd_tt;
      double D1 = (tt2tdb(jd + eps) - tt2tdb(jd - eps)) / (2.0 * eps);
      D = novas_clock_skew(frame, NOVAS_TT);

      // (1 + D) * (1 - D') = 1 + D - (1 + D) D'
      D -= D1 * (1.0 + D);
      break;
    }

    case NOVAS_TCB : {
      double b = novas_vlen(frame->obs_vel) * (AU / DAY) / C; // geocentric motion
      D = b >= 1.0 ? -1.0 : kinetic_potential(b) + sqrt(1.0 - b * b) * solar_system_potential(frame, frame->obs_pos);
      break;
    }

    default:
      novas_set_errno(EINVAL, fn, "timescale %d not supported.", timescale);
      return NAN;
  }

  if(isnan(D))
    return novas_trace_nan(fn);

  return D;
}

/**
 * Returns the averaged incremental rate at which the observer's clock (i.e. proper time &tau;)
 * ticks faster than a clock in the specified timescale. I.e., it returns _D_, which is defined
 * by:
 *
 * d&tau<sub>obs</sub>; / dt<sub>timescale</sub> = (1 + _D_)
 *
 * For a non-Earth-bound observer, this is the same as the instantaneous rate returned by
 * `novas_clock_skew()`. For an Earth-bound observer however, this function returns the averaged
 * value over the observer's rotation around the geocenter. As such it filters out the tiny
 * diurnal or orbital tidal variations as the observer moves through the tidal potential around
 * the geocenter (mainly due to the Sun and Moon). Thus, for Earth-bound observers it returns Eqs.
 * 10.6 and 10.8 of the IERS Conventions.
 *
 * For reduced accuracy frames, the result will be approximate, because the gravitational effect
 * of the Sun and Earth alone may be accounted for.
 *
 * NOTES:
 * <ol>
 * <li>Based on the IERS Conventions 2010, Chapter 10, Eqs. 10.6 / 10.8, but modified for
 * relativistic observer motion.</li>
 * <li>The potential for an observer inside 0.9 planet radii of a major Solar-system body's
 * center will not include the term for that body in the calculation.</li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 * <li>IERS Conventions 2010, Chapter 10, see at https://iers-conventions.obspm.fr/content/chapter10/tn36_c10.pdf</li>
 * </ol>
 *
 * @param frame       The observing frame, defining the observer position as well as the positions
 *                    of the major solar-system bodies at the time of observation.
 * @param timescale   Reference timescale for the comparison. All timescales are supported, except
 *                    `NOVAS_UT1` (since UT1 advances at an irregular rate).
 * @return            The average incremental rate over a geocentric rotation, at which the
 *                    observer's proper time clock ticks faster than the specified timescale, or
 *                    else NAN if the input frame is NULL or uninitialized, or if the timescale is
 *                    not supported (errno set to EINVAL), or if the frame is configured for full
 *                    accuracy but it does not have sufficient planet position information to
 *                    evaluate of the local gravitational potential with precision (errno set to
 *                    EAGAIN).
 *
 * @since 1.5
 * @author Attila Kovacs
 *
 * @sa novas_clock_skew()
 */
double novas_mean_clock_skew(const novas_frame *frame, enum novas_timescale timescale) {
  double D = novas_clock_skew(frame, timescale);
  if(isnan(D))
      return novas_trace_nan("novas_mean_clock_skew");

  // (1 + D) * (1 - D') = 1 + D - (1 + D) D'
  return D - (1.0 + D) * tidal_clock_skew(frame);
}

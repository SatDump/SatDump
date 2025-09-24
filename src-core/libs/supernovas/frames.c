/**
 * @file
 *
 * @date Created  on Jun 23, 2024
 * @author Attila Kovacs
 * @since 1.1
 *
 * SuperNOVAS routines for higher-level and efficient astrometric calculations using observer
 * frames. Observer frames represent an observer location at a specific astronomical time
 * (instant), which can be re-used again and again to calculate or transform positions of
 * celestial sources in a a range of astronomical coordinate systems.
 *
 * To use frames, you start with `novas_make_frame()` with an astrometric time and an observer
 * location, and optionally Earth-orientation parameters if precision below the arcsecond
 * level is needed. E.g.:
 *
 * ```c
 *   observer obs;         // observer location
 *   novas_timespec time;  // astrometric time
 *   novas_frame frame;    // observing frame
 *
 *   // Specify an observer, with GPS coordinates
 *   make_gps_observer(gps_lat, gps_lon, gps_alt, &obs);
 *
 *   // Specify time, e.g. current time with leap seconds and UT1-UTC time difference:
 *   novas_set_current_time(leap_seconds, dut1, &time);
 *
 *   // Set up a frame with the desired accuracy and Earth orientation parameters xp, yp.
 *   novas_make_frame(NOVAS_REDUCED_ACCURACY, &obs, &time, xp, yp, &frame);
 * ```
 *
 * Once a frame is defined, you can perform various astrometric calculation within it
 * efficiently. For example, calculate apparent positions, in the coordinate system of choice, to
 * predict where objects would be seen by the observer:
 *
 * ```c
 *   object source = ...;   // observed source
 *   sky_pos app;           // apparent position to calculate.
 *
 *   // Calculate True-of-Date apparent coordinates for a source.
 *   novas_sky_pos(&source, &frame, NOVAS_TOD, &app);
 * ```
 *
 * If your observer was defined on Earth (ground-based or airborne), you might continue to
 * calculate Az/El positions using `novas_app_to_hor()`, or you might just want to know the same
 * position in another reference system also:
 *
 * ```c
 *   novas_transform T;     // Transformation between reference systems
 *   sky_pos tirs;          // calculated apparent position in TIRS
 *
 *   // Transform from True-of-Date to TIRS coordinates
 *   novas_make_transform(&frame, NOVAS_TOD, NOVAS_TIRS, &T);
 *
 *   // Transform the above position to TIRS...
 *   novas_transform_sky_pos(&app, T, &tirs);
 * ```
 *
 * Or, you can calculate geometric positions, which are not corrected for aberration or
 * gravitational deflection (but still corrected for light travel time in case of Solar-system
 * sources):
 *
 * ```c
 *   object source = ...;   // observed source
 *   double pos[3], vel[3]; // geometric position and velocity vectors to calculate.
 *
 *   // Calculate geometric positions / velocities, say in ICRS
 *   novas_geom_posvel(&source, &frame, NOVAS_ICRS, pos, vel);
 * ```
 *
 * You can also transform geometric positions / velocities to other reference frames using the
 * same `novas_transform` as above.
 *
 * Or, perhaps you are interested when the source will rise (or transit, or set) next, for a
 * ground-based or airborne observer:
 *
 * ```c
 *   object source = ...;   // observed source
 *
 *   // UTC-based Julian date when source rises above 20 degrees given a refraction model.
 *   double jd_rise = novas_rises_above(20.0, &source, &frame, novas_standard_refraction);
 * ```
 *
 * Or, you might simply want to know how far your source is from the Sun or Moon (or another
 * source) on the sky:
 *
 * ```c
 *   object source = ...;   // observed source
 *
 *   // Calculate the angular distance, in degrees, of the source from the Sun.
 *   double angle_deg = novas_sun_angle(&source, &frame);
 * ```
 *
 * These are just some of the common use-case scenarios. There is even more possibilities with
 * frames...
 *
 * @sa timescale.c, observer.c, target.c
 */

/// \cond PRIVATE
#define __NOVAS_INTERNAL_API__      ///< Use definitions meant for internal use by SuperNOVAS only
/// \endcond

#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "novas.h"

/// \cond PRIVATE
#define XI0       (-0.0166170 * ARCSEC)         ///< Frame bias term &xi;<sub>0</sub>
#define ETA0      (-0.0068192 * ARCSEC)         ///< Frame bias term &eta;<sub>0</sub>
#define DA0       (-0.01460 * ARCSEC)           ///< Frame bias term da<sub>0</sub>

#define FRAME_DEFAULT       0                   ///< frame.state value we set to indicate the frame is not configured
#define FRAME_INITIALIZED   0xdeadbeadcafeba5e  ///< frame.state for a properly initialized frame.
#define GEOM_TO_APP         1                   ///< Geometric to apparent conversion
#define APP_TO_GEOM         (-1)                ///< Apparent to geometric conversion

#define NOVAS_TRACK_DELTA   30.0                ///< [s] Time step for evaluation horizontal tracking derivatives.
#define SIDEREAL_RATE       1.002737891         ///< rate at which sidereal time advances faster than UTC
/// \endcond



static int cmp_sys(enum novas_reference_system a, enum novas_reference_system b) {
  // GCRS=0, TOD=1, CIRS=2, ICRS=3, J2000=4, MOD=5, TIRS=6, ITRS=7
  // TOD->-3, MOD->-2, J2000->-1, GCRS/ICRS->0, CIRS->1, TIRS->2, ITRS->3
  static const int index[] = { 0, -3, 1, 0, -1, -2, 2, 3 };

  if(a < 0 || a >= NOVAS_REFERENCE_SYSTEMS)
    return novas_error(-2, EINVAL, "cmp_sys", "Invalid reference system (#1): %d", a);

  if(b < 0 || b >= NOVAS_REFERENCE_SYSTEMS)
    return novas_error(-2, EINVAL, "cmp_sys", "Invalid reference system (#2): %d", b);

  if(index[a] == index[b])
    return 0;

  return index[a] < index[b] ? -1 : 1;
}

static int matrix_transform(const double *in, const novas_matrix *matrix, double *out) {
  double v[3];
  int i;

  memcpy(v, in, sizeof(v));

  for(i = 3; --i >= 0;)
    out[i] = (matrix->M[i][0] * v[0]) + (matrix->M[i][1] * v[1]) + (matrix->M[i][2] * v[2]);

  return 0;
}

static int matrix_inv_rotate(const double *in, const novas_matrix *matrix, double *out) {
  // IMPORTANT! use only with unitary matrices.
  double v[3];
  int i;

  memcpy(v, in, sizeof(v));

  for(i = 3; --i >= 0;)
    out[i] = (matrix->M[0][i] * v[0]) + (matrix->M[1][i] * v[1]) + (matrix->M[2][i] * v[2]);

  return 0;
}

static int invert_matrix(const novas_matrix *A, novas_matrix *I) {
  double idet;
  int i;

  I->M[0][0] = A->M[1][1] * A->M[2][2] - A->M[2][1] * A->M[1][2];
  I->M[1][0] = A->M[2][0] * A->M[1][2] - A->M[1][0] * A->M[2][2];
  I->M[2][0] = A->M[1][0] * A->M[2][1] - A->M[2][0] * A->M[1][1];

  I->M[0][1] = A->M[2][1] * A->M[0][2] - A->M[0][1] * A->M[2][2];
  I->M[1][1] = A->M[0][0] * A->M[2][2] - A->M[2][0] * A->M[0][2];
  I->M[2][1] = A->M[2][0] * A->M[0][1] - A->M[0][0] * A->M[2][1];

  I->M[0][2] = A->M[0][1] * A->M[1][2] - A->M[1][1] * A->M[0][2];
  I->M[1][2] = A->M[1][0] * A->M[0][2] - A->M[0][0] * A->M[1][2];
  I->M[2][2] = A->M[0][0] * A->M[1][1] - A->M[1][0] * A->M[0][1];

  idet = 1.0 / (A->M[0][0] * I->M[0][0] + A->M[0][1] * I->M[1][0] + A->M[0][2] * I->M[2][0]);

  for(i = 3; --i >= 0;) {
    I->M[i][0] *= idet;
    I->M[i][1] *= idet;
    I->M[i][2] *= idet;
  }

  return 0;
}

static int set_spin_matrix(double angle, novas_matrix *T) {
  double c, s;

  memset(T, 0, sizeof(novas_matrix));

  angle *= DEGREE;
  c = cos(angle);
  s = sin(angle);

  // Rotation matrix (non-zero elements only).
  T->M[0][0] = c;
  T->M[0][1] = s;
  T->M[1][0] = -s;
  T->M[1][1] = c;
  T->M[2][2] = 1.0;

  return 0;
}

static int add_transform(novas_transform *transform, const novas_matrix *component, int dir) {
  int i;
  double M0[3][3];

  memcpy(M0, transform->matrix.M, sizeof(M0));

  for(i = 3; --i >= 0;) {
    int j;
    for(j = 3; --j >= 0;) {
      int k;
      double x = 0.0;
      for(k = 3; --k >= 0;)
        x += (dir < 0 ? component->M[k][i] : component->M[i][k]) * M0[k][j];
      transform->matrix.M[i][j] = x;
    }
  }

  return 0;
}

static int set_frame_tie(novas_frame *frame) {
  // 'xi0', 'eta0', and 'da0' are ICRS frame biases in arcseconds taken
  // from IERS (2003) Conventions, Chapter 5.
  static const double ax = ETA0;
  static const double ay = -XI0;
  static const double az = -DA0;
  const double X = ax * ax, Y = ay * ay, Z = az * az;

  novas_matrix *T = &frame->icrs_to_j2000;

  T->M[0][0] = (1.0 - 0.5 * (Y + Z));
  T->M[0][1] = -az;
  T->M[0][2] = ay;

  T->M[1][0] = az;
  T->M[1][1] = (1.0 - 0.5 * (X + Z));
  T->M[1][2] = -ax;

  T->M[2][0] = -ay;
  T->M[2][1] = ax;
  T->M[2][2] = (1.0 - 0.5 * (X + Y));

  return 0;
}

static int set_gcrs_to_cirs(novas_frame *frame) {
  novas_transform T; memset(&T, 0, sizeof(novas_transform));
  novas_matrix tod2cirs; memset(&tod2cirs, 0, sizeof(novas_matrix));

  set_spin_matrix(15.0 * frame->gst - frame->era, &tod2cirs);

  T.matrix = frame->icrs_to_j2000;
  add_transform(&T, &frame->precession, 1);
  add_transform(&T, &frame->nutation, 1);
  add_transform(&T, &tod2cirs, 1);

  frame->gcrs_to_cirs = T.matrix;

  return 0;
}

static int set_precession(novas_frame *frame) {
  // 't' is time in TDB centuries between the two epochs.
  const double t = (novas_get_time(&frame->time, NOVAS_TDB) - NOVAS_JD_J2000) / JULIAN_CENTURY_DAYS;
  const double eps0 = 84381.406 * ARCSEC;

  // Numerical coefficients of psi_a, omega_a, and chi_a, along with
  // epsilon_0, the obliquity at J2000.0, are 4-angle formulation from
  // Capitaine et al. (2003), eqs. (4), (37), & (39).
  const double psia = ((((-0.0000000951 * t + 0.000132851) * t - 0.00114045) * t - 1.0790069) * t + 5038.481507) * t * ARCSEC;
  const double omegaa = ((((+0.0000003337 * t - 0.000000467) * t - 0.00772503) * t + 0.0512623) * t - 0.025754) * t * ARCSEC + eps0;
  const double chia = ((((-0.0000000560 * t + 0.000170663) * t - 0.00121197) * t - 2.3814292) * t + 10.556403) * ARCSEC * t;

  // P03rev2 / Capitaine at al. (2005) eqs. (11)
  //const double psia = t * (5038.482090 + t * (-1.0789921 + t * (-0.00114040 + t * (0.000132851 - t * 0.0000000951)))) * ARCSEC;
  //const double omegaa = eps0 + t * (-0.025675 + t * (0.0512622 + t * (-0.00772501 + t * (-0.000000467 + t * 0.0000003337)))) * ARCSEC;

  // Liu & Capitaine (2017)
  //const double chia = t * (10.556240 + t * (-2.3813876 + t * (-0.00121311 + t * (0.000160286 + t * 0.000000086)))) * ARCSEC;
  //const double psia = t * (5038.481270 + t * (-1.0732468 + t * (0.01573403 + t * (0.000127135 - t * 0.0000001020)))) * ARCSEC;
  //const double omegaa = eps0 + t * (-0.024725 + t * (0.0512626 + t * (-0.0077249 + t * (-0.000000267 + t * 0.000000267)))) * ARCSEC;

  // Liu, Capitaine, & Cheng (2019)
  // Another update on LC17 with new VLBI data
  //const double chia = t * (10.556240 + t * (-2.3813876 + t * (-0.00121311 + t * (0.000160286 + t * 0.000000086))));
  //const double psia = t * (5038.482041 + t * (-1.072687 + t * (0.0278555 + t * (0.00012342 - t * 0.0000001096))));
  //const double omegaa = eps0 + t * (-0.025754 + t * (0.0512626 + t * (-0.0077249 + t * (-0.000000086 + t * 0.000000221))));

  const double sa = sin(eps0);
  const double ca = cos(eps0);
  const double sb = sin(-psia);
  const double cb = cos(-psia);
  const double sc = sin(-omegaa);
  const double cc = cos(-omegaa);
  const double sd = sin(chia);
  const double cd = cos(chia);

  double t1, t2;

  novas_matrix *T = &frame->precession;

  // Compute elements of precession rotation matrix equivalent to
  // R3(chi_a) R1(-omega_a) R3(-psi_a) R1(epsilon_0).
  t1 = cd * sb + sd * cc * cb;
  t2 = sd * sc;
  T->M[0][0] = cd * cb - sb * sd * cc;
  T->M[0][1] = ca * t1 - sa * t2;
  T->M[0][2] = sa * t1 + ca * t2;

  t1 = cd * cc * cb - sd * sb;
  t2 = cd * sc;
  T->M[1][0] = -sd * cb - sb * cd * cc;
  T->M[1][1] = ca * t1 - sa * t2;
  T->M[1][2] = sa * t1 + ca * t2;

  T->M[2][0] = sb * sc;
  T->M[2][1] = -sc * cb * ca - sa * cc;
  T->M[2][2] = -sc * cb * sa + cc * ca;

  return 0;
}

static int set_nutation(novas_frame *frame) {
  const double cm = cos(frame->mobl);
  const double sm = sin(frame->mobl);
  const double ct = cos(frame->tobl);
  const double st = sin(frame->tobl);
  const double cp = cos(frame->dpsi0);
  const double sp = sin(frame->dpsi0);

  novas_matrix *T = &frame->nutation;

  // Nutation rotation matrix follows.
  T->M[0][0] = cp;
  T->M[0][1] = -sp * cm;
  T->M[0][2] = -sp * sm;

  T->M[1][0] = sp * ct;
  T->M[1][1] = cp * cm * ct + sm * st;
  T->M[1][2] = cp * sm * ct - cm * st;

  T->M[2][0] = sp * st;
  T->M[2][1] = cp * cm * st - sm * ct;
  T->M[2][2] = cp * sm * st + cm * ct;

  return 0;
}



static int set_wobble_matrix(const novas_frame *frame, novas_matrix *T) {
  // TIRS / PEF -> ITRS

  // Compute approximate longitude of TIO (s'), using eq. (10) of the second reference
  const double t = (frame->time.ijd_tt + frame->time.fjd_tt - JD_J2000) / JULIAN_CENTURY_DAYS;

  const double ax = frame->dy * MAS;
  const double ay = frame->dx * MAS;
  const double az = 47.0e-6 * ARCSEC * t;   // -s1

  const double A[3] = { ax * ax, ay * ay, az * az };

  //memset(T, 0, sizeof(novas_matrix));

  T->M[0][0] = 1.0 - 0.5 * (A[1] + A[2]);
  T->M[0][1] = -az;
  T->M[0][2] = ay;

  T->M[1][0] = az + (ax * ay);
  T->M[1][1] = 1.0 - 0.5 * (A[0] + A[2]);
  T->M[1][2] = -ax;

  T->M[2][0] = -ay;
  T->M[2][1] = ax;
  T->M[2][2] = 1.0 - 0.5 * (A[0] + A[1]);

  return 0;
}

static int set_obs_posvel(novas_frame *frame) {
  int res = obs_posvel(novas_get_time(&frame->time, NOVAS_TDB), frame->time.ut1_to_tt, frame->accuracy, &frame->observer,
          frame->earth_pos, frame->earth_vel, frame->obs_pos, frame->obs_vel);

  frame->v_obs = novas_vlen(frame->obs_vel);
  frame->beta = frame->v_obs / C_AUDAY;
  frame->gamma = sqrt(1.0 - frame->beta * frame->beta);
  return res;
}


static int frame_aberration(const novas_frame *frame, int dir, double *pos) {
  const double pos0[3] = { pos[0], pos[1], pos[2] };
  double d;
  int i;

  if(frame->v_obs == 0.0)
    return 0;

  d = novas_vlen(pos);
  if(d == 0.0)
    return 0;

  // Iterate as necessary (for inverse only)
  for(i = 0; i < novas_inv_max_iter; i++) {
    const double p = frame->beta * novas_vdot(pos, frame->obs_vel) / (d * frame->v_obs);
    const double q = (1.0 + p / (1.0 + frame->gamma)) * d / C_AUDAY;
    const double r = 1.0 + p;

    if(dir < 0) {
      const double pos1[3] = { pos[0], pos[1], pos[2] };

      // Apparent to geometric
      pos[0] = (r * pos0[0] - q * frame->obs_vel[0]) / frame->gamma;
      pos[1] = (r * pos0[1] - q * frame->obs_vel[1]) / frame->gamma;
      pos[2] = (r * pos0[2] - q * frame->obs_vel[2]) / frame->gamma;

      // Iterate, since p, q, r are defined by unaberrated position.
      if(novas_vdist(pos, pos1) < 1e-13 * d)
        return 0;
    }
    else {
      // Geometric to apparent
      pos[0] = (frame->gamma * pos0[0] + q * frame->obs_vel[0]) / r;
      pos[1] = (frame->gamma * pos0[1] + q * frame->obs_vel[1]) / r;
      pos[2] = (frame->gamma * pos0[2] + q * frame->obs_vel[2]) / r;
      return 0; // No need to iterate...
    }
  }

  return novas_error(-1, ECANCELED, "frame_aberration", "failed to converge");
}

/// \cond PRIVATE
/**
 * Checks if a frame has been initialized either via a call to `make_frame()`.
 *
 * @param frame   The observing frame, defining the time of observation and the observer location.
 *                !!! It cannot be NULL !!!
 * @return        boolean TRUE (1) if the frame has been initialized, or else FALSE (0).
 *
 * @sa novas_make_frame()
 */
int novas_frame_is_initialized(const novas_frame *frame) {
  //if(!frame) return 0;
  return frame->state == FRAME_INITIALIZED;
}
/// \endcond

/**
 * Sets up a observing frame for a specific observer location, time of observation, and accuracy
 * requirement. The frame is initialized using the currently configured planet ephemeris provider
 * function (see set_planet_provider() and set_planet_provider_hp()), and in case of reduced
 * accuracy mode, the currently configured IAU nutation model provider (see
 * set_nutation_lp_provider()).
 *
 * Note, that to construct full accuracy frames, you will need a high-precision ephemeris provider
 * for the major planets (not just the default Earth/Sun), as without it, gravitational bending
 * around massive plannets cannot be accounted for, and therefore &mu;as accuracy cannot be ensured,
 * in general. Attempting to construct a high-accuracy frame without a high-precision ephemeris
 * provider for the major planets will result in an error in the 10--40 range from the required
 * ephemeris() call.
 *
 * NOTES:
 * <ol>
 * <li>This function expects the Earth polar wobble parameters to be defined on a per-frame basis
 * and will not use the legacy global (undated) orientation parameters set via cel_pole().</li>
 * <li>The Earth orientation parameters xp, yp should be provided in the same ITRF realization as
 * the observer location for an Earth-based observer. You can use `novas_itrf_transform_eop()` to
 * convert the EOP values as necessary.</li>
 * </ol>
 *
 * @param accuracy    Accuracy requirement, NOVAS_FULL_ACCURACY (0) for the utmost precision or
 *                    NOVAS_REDUCED_ACCURACY (1) if ~1 mas accuracy is sufficient.
 * @param obs         Observer location
 * @param time        Time of observation
 * @param xp          [mas] Earth orientation parameter, polar offset in _x_, e.g. from the IERS
 *                    Bulletins, and possibly corrected for diurnal and semi-diurnal variations,
 *                    e.g. via `novas_diurnal_eop()`. (The global, undated value set by cel_pole()
 *                    is not not used here.) You can use 0.0 if sub-arcsecond accuracy is not
 *                    required.
 * @param yp          [mas] Earth orientation parameter, polar offset in _y_, e.g. from the IERS
 *                    Bulletins, and possibly corrected for diurnal and semi-diurnal variations,
 *                    e.g. via `novas_diurnal_eop()`. (The global, undated value set by cel_pole()
 *                    is not not used here.) You can use 0.0 if sub-arcsecond accuracy is not
 *                    required.
 * @param[out] frame  Pointer to the observing frame to configure.
 * @return            0 if successful,
 *                    10--40: error is 10 + the error from ephemeris(),
 *                    40--50: error is 40 + the error from geo_posvel(),
 *                    or else -1 if there was an error (errno will indicate the type of error).
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa novas_change_observer(), novas_sky_pos(), novas_geom_posvel(), novas_make_transform()
 * @sa set_planet_provider(), set_planet_provider_hp(), set_nutation_lp_provider(),
 *     novas_diurnal_eop(), novas_itrf_transform_eop()
 */
int novas_make_frame(enum novas_accuracy accuracy, const observer *obs, const novas_timespec *time, double xp, double yp,
        novas_frame *frame) {
  static const char *fn = "novas_make_frame";
  static const object earth = NOVAS_EARTH_INIT;
  static const object sun = NOVAS_SUN_INIT;

  double tdb2[2];
  double dpsi, deps;
  long ijd_ut1;
  double fjd_ut1;

  if(accuracy < 0 || accuracy > NOVAS_REDUCED_ACCURACY)
    return novas_error(-1, EINVAL, fn, "invalid accuracy: %d", accuracy);

  if(!obs || !time)
    return novas_error(-1, EINVAL, fn, "NULL input parameter: obs=%p, time=%p", obs, time);

  if(!frame)
    return novas_error(-1, EINVAL, fn, "NULL output frame");

  if(obs->where < 0 || obs->where >= NOVAS_OBSERVER_PLACES)
    return novas_error(-1, EINVAL, fn, "invalid observer location: %d", obs->where);

  frame->accuracy = accuracy;
  frame->time = *time;

  tdb2[0] = time->ijd_tt;
  tdb2[1] = time->fjd_tt + tt2tdb(time->ijd_tt + time->fjd_tt) / DAY;

  nutation_angles((tdb2[0] + tdb2[1] - NOVAS_JD_J2000) / JULIAN_CENTURY_DAYS, accuracy, &dpsi, &deps);

  // dpsi0 / dpes0 w/o the global pole offsets set via cel_pole()
  frame->dpsi0 = dpsi * ARCSEC;
  frame->deps0 = deps * ARCSEC;
  frame->dx = xp;
  frame->dy = yp;

  // Compute mean obliquity of the ecliptic in degrees.
  frame->mobl = mean_obliq(tdb2[0] + tdb2[1]) * ARCSEC;

  // Obtain complementary terms for equation of the equinoxes in radians.
  frame->ee = dpsi * ARCSEC * cos(frame->mobl) + ee_ct(time->ijd_tt, time->fjd_tt, accuracy);

  // Compute true obliquity of the ecliptic in degrees.
  frame->tobl = frame->mobl + deps * ARCSEC;

  fjd_ut1 = novas_get_split_time(time, NOVAS_UT1, &ijd_ut1);
  frame->era = era(ijd_ut1, fjd_ut1);

  //frame->gst = novas_gast(ijd_ut1 + fjd_ut1, time->ut1_to_tt, accuracy); // Use faster calc with available quantities.
  frame->gst = (frame->era + novas_gmst_prec(tdb2[0] + tdb2[1]) / 3600.0) / 15.0 + frame->ee / HOURANGLE;
  frame->gst = remainder(frame->gst, DAY_HOURS);
  if(frame->gst < 0) frame->gst += DAY_HOURS;

  set_frame_tie(frame);
  set_precession(frame);
  set_nutation(frame);
  set_gcrs_to_cirs(frame);

  // Barycentric Earth and Sun positions and velocities
  prop_error(fn, ephemeris(tdb2, &sun, NOVAS_BARYCENTER, accuracy, frame->sun_pos, frame->sun_vel), 10);
  prop_error(fn, ephemeris(tdb2, &earth, NOVAS_BARYCENTER, accuracy, frame->earth_pos, frame->earth_vel), 10);

  frame->state = FRAME_INITIALIZED;

  prop_error(fn, novas_change_observer(frame, obs, frame), 40);

  return 0;
}

/**
 * Change the observer location for an observing frame.
 *
 * @param orig        Pointer to original observing frame
 * @param obs         New observer location
 * @param[out] out    Observing frame to populate with a original frame data and new observer
 *                    location. It can be the same as the input.
 * @return            0 if successfule or else an an error code from geo_posvel()
 *                    (errno will also indicate the type of error).
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa novas_make_frame()
 */
int novas_change_observer(const novas_frame *orig, const observer *obs, novas_frame *out) {
  static const char *fn = "novas_change_observer";
  double jd_tdb;
  int pl_mask;

  if(!orig || !obs || !out)
    return novas_error(-1, EINVAL, fn, "NULL parameter: orig=%p, obs=%p, out=%p", orig, obs, out);

  if(!novas_frame_is_initialized(orig))
    return novas_error(-1, EINVAL, fn, "frame at %p not initialized", out);

  if(out != orig)
    *out = *orig;

  out->state = FRAME_DEFAULT;
  out->observer = *obs;

  pl_mask = (out->accuracy == NOVAS_FULL_ACCURACY) ? grav_bodies_full_accuracy : grav_bodies_reduced_accuracy;

  prop_error(fn, set_obs_posvel(out), 0);

  jd_tdb = novas_get_time(&out->time, NOVAS_TDB);
  prop_error(fn, obs_planets(jd_tdb, out->accuracy, out->obs_pos, pl_mask, &out->planets), 0);

  out->state = FRAME_INITIALIZED;
  return 0;
}

static int icrs_to_sys(const novas_frame *restrict frame, double *restrict pos, enum novas_reference_system sys) {
  switch(sys) {
    case NOVAS_ICRS:
    case NOVAS_GCRS:
      return 0;

    case NOVAS_CIRS:
    case NOVAS_TIRS:
    case NOVAS_ITRS:
      matrix_transform(pos, &frame->gcrs_to_cirs, pos);
      if(sys == NOVAS_CIRS) return 0;

      spin(frame->era, pos, pos);
      if(sys == NOVAS_TIRS) return 0;

      wobble(frame->time.ijd_tt + frame->time.fjd_tt, WOBBLE_TIRS_TO_ITRS, frame->dx, frame->dy, pos, pos);
      return 0;

    case NOVAS_J2000:
    case NOVAS_MOD:
    case NOVAS_TOD:
      //gcrs_to_j2000(pos, pos);
      matrix_transform(pos, &frame->icrs_to_j2000, pos);
      if(sys == NOVAS_J2000) return 0;

      matrix_transform(pos, &frame->precession, pos);
      if(sys == NOVAS_MOD) return 0;

      matrix_transform(pos, &frame->nutation, pos);
      return 0;
  }

  return novas_error(-1, EINVAL, "icrs_to_sys", "invalid reference system: %d", sys);
}

/**
 * Calculates the geometric position and velocity vectors, relative to the observer, for a source
 * in the given observing frame, in the specified coordinate system of choice. The geometric
 * position includes proper motion, and for solar-system bodies it is antedated for light travel
 * time, so it effectively represents the geometric position as seen by the observer. However, the
 * geometric does not include aberration correction, nor gravitational deflection.
 *
 * If you want apparent positions, which account for aberration and gravitational deflection,
 * use novas_skypos() instead.
 *
 * You can also use novas_transform_vector() to convert the output position and velocity vectors
 * to a dfferent coordinate system of choice afterwards if you want the results expressed in
 * more than one coordinate system.
 *
 * It implements the same geometric transformations as `place()` but at a reduced computational
 * cost. See `place()` for references.
 *
 * NOTES:
 * <ol>
 * <li>As of SuperNOVAS v1.3, the returned velocity vector is a proper observer-based
 * velocity measure. In prior releases, and in NOVAS C 3.1, this was inconsistent, with
 * pseudo LSR-based measures being returned for catalog sources.</li>
 * </ol>
 *
 * @param source        Pointer to a celestial source data structure that is observed. Catalog
 *                      sources should have coordinates and properties in ICRS. You can use
 *                      `transform_cat()` to convert catalog entries to ICRS as necessary.
 * @param frame         Observer frame, defining the location and time of observation
 * @param sys           The coordinate system in which to return positions and velocities.
 * @param[out] pos      [AU] Calculated geometric position vector of the source relative
 *                      to the observer location, in the designated coordinate system. It may be
 *                      NULL if not required.
 * @param[out] vel      [AU/day] The calculated velocity vector of the source relative to
 *                      the observer in the designated coordinate system. It must be distinct from
 *                      the pos output vector, and may be NULL if not required.
 * @return              0 if successful, or else -1 if any of the arguments is invalid,
 *                      50--70 error is 50 + error from light_time2().
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa novas_geom_to_app(), novas_sky_pos(),
 * @sa novas_transform_vector(), novas_make_frame()
 */
int novas_geom_posvel(const object *restrict source, const novas_frame *restrict frame, enum novas_reference_system sys,
        double *restrict pos, double *restrict vel) {
  static const char *fn = "novas_geom_posvel";

  double jd_tdb, t_light;
  double pos1[3], vel1[3];

  if(!source || !frame)
    return novas_error(-1, EINVAL, fn, "NULL argument: source=%p, frame=%p", source, frame);

  if(pos == vel)
    return novas_error(-1, EINVAL, fn, "identical output pos and vel 3-vectors @ %p.", pos);

  if(!novas_frame_is_initialized(frame))
    return novas_error(-1, EINVAL, fn, "frame at %p not initialized", frame);

  if(frame->accuracy != NOVAS_FULL_ACCURACY && frame->accuracy != NOVAS_REDUCED_ACCURACY)
    return novas_error(-1, EINVAL, fn, "invalid accuracy: %d", frame->accuracy);
  // Compute 'jd_tdb', the TDB Julian date corresponding to 'jd_tt'.
  jd_tdb = novas_get_time(&frame->time, NOVAS_TDB);

  // ---------------------------------------------------------------------
  // Find geometric position of observed object (ICRS)
  // ---------------------------------------------------------------------
  if(source->type == NOVAS_CATALOG_OBJECT) {
    // Observed object is star.
    double dt = 0.0;

    // Get position of star updated for its space motion
    // (The motion calculated here is not used for radial velocity in `rad_vel2()`)
    starvectors(&source->star, pos1, vel1);

    dt = d_light(pos1, frame->obs_pos);
    proper_motion(NOVAS_JD_J2000, pos1, vel1, (jd_tdb + dt), pos1);

    // Get position of star wrt observer (corrected for parallax).
    bary2obs(pos1, frame->obs_pos, pos1, &t_light);
  }
  else {
    int got = 0;

    // If we readily have the requested planet data in the frame, use it.
    if(source->type == NOVAS_PLANET)
      if(frame->planets.mask & (1 << source->number)) {
        memcpy(pos1, &frame->planets.pos[source->number][0], sizeof(pos1));
        memcpy(vel1, &frame->planets.vel[source->number][0], sizeof(vel1));
        got = 1;
      }

    // Otherwise, get the position of body wrt observer, antedated for light-time.
    if(!got) {
      prop_error(fn, light_time2(jd_tdb, source, frame->obs_pos, 0.0, frame->accuracy, pos1, vel1, &t_light), 50);
    }
  }

  if(pos) {
    prop_error(fn, icrs_to_sys(frame, pos1, sys), 0);
    memcpy(pos, pos1, sizeof(pos1));
  }
  if(vel) {
    prop_error(fn, icrs_to_sys(frame, vel1, sys), 0);
    memcpy(vel, vel1, sizeof(vel1));
  }

  return 0;
}

/**
 * Calculates an apparent location on sky for the source. The position takes into account the
 * proper motion (for sidereal source), or is antedated for light-travel time (for Solar-System
 * bodies). It also applies an appropriate aberration correction and gravitational deflection of
 * the light.
 *
 * To calculate corresponding local horizontal coordinates, you can pass the output RA/Dec
 * coordinates to novas_app_to_hor(). Or to calculate apparent coordinates in other systems,
 * you may pass the result to novas_transform_sy_pos() after.
 *
 * And if you want geometric positions instead (not corrected for aberration or gravitational
 * deflection), you may want to use novas_geom_posvel() instead.
 *
 * The approximate 'inverse' of this function is novas_app_to_geom().
 *
 * This function implements the same aberration and gravitational deflection corrections as
 * `place()`, but at reduced computational cost. See `place()` for references. Unlike `place()`,
 * however, the output always reports the distance calculated from the parallax for sidereal
 * sources. Note also, that while `place()` does not apply aberration and gravitational deflection
 * corrections when `sys` is NOVAS_ICRS (3), this routine will apply those corrections consistently
 * for all coordinate systems (and you can use novas_geom_posvel() instead to get positions without
 * aberration or deflection in any system).
 *
 * NOTES:
 * <ol>
 * <li>As of SuperNOVAS v1.3, the returned radial velocity component is a proper observer-based
 * spectroscopic measure. In prior releases, and in NOVAS C 3.1, this was inconsistent, with
 * LSR-based measures being returned for catalog sources.</li>
 * </ol>
 *
 * @param object        Pointer to a celestial object data structure that is observed. Catalog
 *                      sources should have coordinates and properties in ICRS. You can use
 *                      `transform_cat()` to convert catalog entries to ICRS as necessary.
 * @param frame         The observer frame, defining the location and time of observation.
 * @param sys           The coordinate system in which to return the apparent sky location.
 * @param[out] out      Pointer to the data structure which is populated with the calculated
 *                      apparent location in the designated coordinate system.
 * @return              0 if successful,
 *                      50--70 error is 50 + error from light_time2(),
 *                      70--80 error is 70 + error from grav_def(),
 *                      or else -1 (errno will indicate the type of error).
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa novas_geom_to_app(), novas_app_to_hor(), novas_make_frame()
 */
int novas_sky_pos(const object *restrict object, const novas_frame *restrict frame, enum novas_reference_system sys,
        sky_pos *restrict out) {
  static const char *fn = "novas_sky_pos";

  double d_sb, pos[3], vel[3], vpos[3];

  if(!object || !frame || !out)
    return novas_error(-1, EINVAL, "NULL argument: object=%p, frame=%p, out=%p", (void *) object, frame, out);

  if(!novas_frame_is_initialized(frame))
    return novas_error(-1, EINVAL, fn, "frame at %p not initialized", frame);

  if(frame->accuracy != NOVAS_FULL_ACCURACY && frame->accuracy != NOVAS_REDUCED_ACCURACY)
    return novas_error(-1, EINVAL, fn, "invalid accuracy: %d", frame->accuracy);

  prop_error(fn, novas_geom_posvel(object, frame, NOVAS_ICRS, pos, vel), 0);

  out->dis = novas_vlen(pos);

  // ---------------------------------------------------------------------
  // Compute radial velocity (all vectors in ICRS).
  // ---------------------------------------------------------------------
  if(object->type == NOVAS_CATALOG_OBJECT) {
    d_sb = out->dis;
  }
  else {
    int k;

    // Calculate distance to Sun.
    d_sb = 0.0;
    for(k = 3; --k >= 0;) {
      double d = frame->sun_pos[k] - (frame->obs_pos[k] + pos[k]);
      d_sb += d * d;
    }
    d_sb = sqrt(d_sb);
  }

  // ---------------------------------------------------------------------
  // Compute direction in which light is emitted from the source
  // ---------------------------------------------------------------------
  if(object->type == NOVAS_CATALOG_OBJECT) {
    // For sidereal sources the 'velocity' position is the same as the geometric position.
    memcpy(vpos, pos, sizeof(pos));
  }
  else {
    double psrc[3]; // Barycentric position of Solar-system source (antedated)
    int i;

    // A.K.: For this we calculate gravitational deflection of the observer seen from the source
    // i.e., reverse tracing the light to find the direction in which it was emitted.
    for(i = 3; --i >= 0;) {
      vpos[i] = -pos[i];
      psrc[i] = pos[i] + frame->obs_pos[i];
    }

    // vpos -> deflected direction in which observer is seen from source.
    prop_error(fn, grav_planets(vpos, psrc, &frame->planets, vpos), 70);

    // vpos -> direction in which light was emitted from observer's perspective...
    for(i = 3; --i >= 0;)
      vpos[i] = -vpos[i];
  }

  prop_error(fn, novas_geom_to_app(frame, pos, sys, out), 70);

  out->rv = rad_vel2(object, vpos, vel, pos, frame->obs_vel, novas_vdist(frame->obs_pos, frame->earth_pos),
          novas_vdist(frame->obs_pos, frame->sun_pos), d_sb);

  return 0;
}

/**
 * Converts an geometric position in ICRS to an apparent position on sky, by applying appropriate
 * corrections for aberration and gravitational deflection for the observer's frame. Unlike
 * `place()` the output reports the distance calculated from the parallax for sidereal sources.
 * The radial velocity of the output is set to NAN (if needed use novas_sky_pos() instead).
 *
 * @param frame     The observer frame, defining the location and time of observation
 * @param pos       [AU] Geometric position of source in ICRS coordinates
 * @param sys       The coordinate system in which to return the apparent sky location
 * @param[out] out  Pointer to the data structure which is populated with the calculated apparent
 *                  location in the designated coordinate system. It may be the same pounter as
 *                  the input position.
 * @return          0 if successful, or an error from grav_def2(), or else -1 (errno will indicate
 *                  the type of error).
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa novas_app_to_geom()
 * @sa novas_app_to_hor(), novas_sky_pos(), novas_geom_posvel()
 */
int novas_geom_to_app(const novas_frame *restrict frame, const double *restrict pos, enum novas_reference_system sys,
        sky_pos *restrict out) {
  const char *fn = "novas_geom_to_app";
  double pos1[3];
  int i;

  if(!pos || !frame || !out)
    return novas_error(-1, EINVAL, "NULL argument: pos=%p, frame=%p, out=%p", (void *) pos, frame, out);

  if(!novas_frame_is_initialized(frame))
    return novas_error(-1, EINVAL, fn, "frame at %p not initialized", frame);

  if(frame->accuracy < NOVAS_FULL_ACCURACY || frame->accuracy > NOVAS_REDUCED_ACCURACY)
    return novas_error(-1, EINVAL, fn, "invalid accuracy: %d", frame->accuracy);

  // Compute gravitational deflection and aberration.
  prop_error(fn, grav_planets(pos, frame->obs_pos, &frame->planets, pos1), 0);

  // Aberration correction
  frame_aberration(frame, GEOM_TO_APP, pos1);

  // Transform position to output system
  prop_error(fn, icrs_to_sys(frame, pos1, sys), 0);

  vector2radec(pos1, &out->ra, &out->dec);

  out->dis = novas_vlen(pos1);
  out->rv = NAN;

  for(i = 3; --i >= 0;)
    out->r_hat[i] = pos1[i] / out->dis;

  return 0;

}

/**
 * Converts an observed apparent position vector in the specified coordinate system to local
 * horizontal coordinates in the specified observer frame. The observer must be located on the
 * surface of Earth, or else the call will return with an error. The caller may optionally supply
 * a refraction model of choice to calculate an appropriate elevation angle that includes a
 * refraction correction for Earth's atmosphere. If no such model is provided the calculated
 * elevation will be the astrometric elevation without a refraction correction.
 *
 * @param frame       Observer frame, defining the time and place of observation (on Earth).
 * @param sys         Astronomical coordinate system in which the observed position is given.
 * @param ra          [h] Observed apparent right ascension (R.A.) coordinate
 * @param dec         [deg] Observed apparent declination coordinate
 * @param ref_model   An appropriate refraction model, or NULL to calculate unrefracted elevation.
 *                    Depending on the refraction model, you might want to make sure that the
 *                    weather parameters were set when the observing frame was defined.
 * @param[out] az     [deg] Calculated azimuth angle. It may be NULL if not required.
 * @param[out] el     [deg] Calculated elevation angle. It may be NULL if not required.
 * @return            0 if successful, or else an error from tod_to_itrs() or cirs_to_itrs(), or
 *                    -1 (errno will indicate the type of error).
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa novas_hor_to_app()
 * @sa novas_app_to_geom(), novas_standard_refraction(), novas_optical_refraction(),
 *     novas_radio_refraction(), novas_wave_refraction()
 */
int novas_app_to_hor(const novas_frame *restrict frame, enum novas_reference_system sys, double ra, double dec,
        RefractionModel ref_model, double *restrict az, double *restrict el) {
  static const char *fn = "novas_app_to_hor";
  const novas_timespec *time;
  double pos[3];
  double az0, za0;

  if(!frame)
    return novas_error(-1, EINVAL, fn, "NULL observing frame");

  if(!az && !el) {
    return novas_error(-1, EINVAL, fn, "Both output pointers (az, el) are NULL");
  }

  if(!novas_frame_is_initialized(frame))
    return novas_error(-1, EINVAL, fn, "frame at %p not initialized", frame);

  if(frame->observer.where != NOVAS_OBSERVER_ON_EARTH && frame->observer.where != NOVAS_AIRBORNE_OBSERVER) {
    return novas_error(-1, EINVAL, fn, "observer not on Earth: where=%d", frame->observer.where);
  }

  time = (novas_timespec *) &frame->time;

  radec2vector(ra, dec, 1.0, pos);

  switch(sys) {
    case NOVAS_J2000:
      matrix_transform(pos, &frame->precession, pos); // @suppress("No break at end of case")
      /* fallthrough */
    case NOVAS_MOD:
      matrix_transform(pos, &frame->nutation, pos); // @suppress("No break at end of case")
      /* fallthrough */
    case NOVAS_TOD:
      spin(15.0 * frame->gst, pos, pos);
      wobble(time->ijd_tt + time->fjd_tt, WOBBLE_PEF_TO_ITRS, 1e-3 * frame->dx, 1e-3 * frame->dy, pos, pos);
      break;

    case NOVAS_ICRS:
    case NOVAS_GCRS:
      matrix_transform(pos, &frame->gcrs_to_cirs, pos); // @suppress("No break at end of case")
      /* fallthrough */
    case NOVAS_CIRS:
      spin(frame->era, pos, pos); // @suppress("No break at end of case")
      /* fallthrough */
    case NOVAS_TIRS:
      wobble(time->ijd_tt + time->fjd_tt, WOBBLE_TIRS_TO_ITRS, 1e-3 * frame->dx, 1e-3 * frame->dy, pos, pos); // @suppress("No break at end of case")
      /* fallthrough */
    case NOVAS_ITRS:
      break;

    default:
      return novas_error(-1, EINVAL, fn, "invalid coordinate system: %d", sys);
  }

  itrs_to_hor(&frame->observer.on_surf, pos, &az0, &za0);

  if(ref_model)
    za0 -= ref_model(time->ijd_tt + time->fjd_tt, &frame->observer.on_surf, NOVAS_REFRACT_ASTROMETRIC, 90.0 - za0);

  if(az)
    *az = az0;
  if(el)
    *el = 90.0 - za0;

  return 0;
}

/**
 * Converts an observed azimuth and elevation coordinate to right ascension (R.A.) and declination
 * coordinates expressed in the coordinate system of choice. The observer must be located on the
 * surface of Earth, or else the call will return with an error. The caller may optionally supply
 * a refraction model of choice to calculate an appropriate elevation angle that includes a
 * refraction correction for Earth's atmosphere. If no such model is provided, the provided
 * elevation value will be assumed to be an astrometric elevation without a refraction correction.
 *
 * @param frame       Observer frame, defining the time and place of observation (on Earth).
 * @param az          [deg] Observed azimuth angle. It may be NULL if not required.
 * @param el          [deg] Observed elevation angle. It may be NULL if not required.
 * @param ref_model   An appropriate refraction model, or NULL to assume unrefracted elevation.
 *                    Depending on the refraction model, you might want to make sure that the
 *                    weather parameters were set when the observing frame was defined.
 * @param sys         Astronomical coordinate system in which the output is R.A. and declination
 *                    values are to be calculated.
 * @param[out] ra     [h] Calculated apparent right ascension (R.A.) coordinate
 * @param[out] dec    [deg] Calculated apparent declination coordinate
 * @return            0 if successful, or else an error from itrs_to_tod() or itrs_to_cirs(), or
 *                    -1 (errno will indicate the type of error).
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa novas_app_to_hor()
 * @sa novas_app_to_geom(), novas_standard_refraction(), novas_optical_refraction(),
 *     novas_radio_refraction(), novas_wave_refraction()
 */
int novas_hor_to_app(const novas_frame *restrict frame, double az, double el, RefractionModel ref_model,
        enum novas_reference_system sys, double *restrict ra, double *restrict dec) {
  static const char *fn = "novas_hor_to_app";
  const novas_timespec *time;
  double pos[3];

  if(!frame)
    return novas_error(-1, EINVAL, fn, "NULL observing frame");

  if(!ra && !dec) {
    return novas_error(-1, EINVAL, fn, "Both output pointers (ra, dec) are NULL");
  }

  if(!novas_frame_is_initialized(frame))
    return novas_error(-1, EINVAL, fn, "frame at %p not initialized", frame);

  if(frame->observer.where != NOVAS_OBSERVER_ON_EARTH && frame->observer.where != NOVAS_AIRBORNE_OBSERVER) {
    return novas_error(-1, EINVAL, fn, "observer not on Earth: where=%d", frame->observer.where);
  }

  time = (novas_timespec *) &frame->time;

  if(ref_model)
    el -= ref_model(time->ijd_tt + time->fjd_tt, &frame->observer.on_surf, NOVAS_REFRACT_OBSERVED, el);

  // az, el to ITRS pos
  hor_to_itrs(&frame->observer.on_surf, az, 90.0 - el, pos);

  if(sys != NOVAS_ITRS) {
    // ITRS -> PEF / TIRS
    enum novas_wobble_direction dir = cmp_sys(sys, NOVAS_GCRS) < 0 ? WOBBLE_ITRS_TO_PEF : WOBBLE_ITRS_TO_TIRS;
    wobble(time->ijd_tt + time->fjd_tt, dir, 1e-3 * frame->dx, 1e-3 * frame->dy, pos, pos);

    if(sys != NOVAS_TIRS) {
      // TIRS -> TOD or CIRS...
      spin(cmp_sys(sys, NOVAS_GCRS) < 0 ? -15.0 * frame->gst : -frame->era, pos, pos);

      // Continue to convert TOD / CIRS to output system....
      switch(sys) {

        case NOVAS_TOD:
          break;

        case NOVAS_MOD:
          matrix_inv_rotate(pos, &frame->nutation, pos);
          break;

        case NOVAS_J2000:
          matrix_inv_rotate(pos, &frame->nutation, pos);
          matrix_inv_rotate(pos, &frame->precession, pos);
          break;

        case NOVAS_CIRS:
          break;

        case NOVAS_ICRS:
        case NOVAS_GCRS:
          matrix_inv_rotate(pos, &frame->gcrs_to_cirs, pos);
          break;

        default:
          return novas_error(-1, EINVAL, fn, "invalid coordinate system: %d", sys);
      }
    }
  }

  vector2radec(pos, ra, dec);
  return 0;
}

/**
 * Converts an observed apparent sky position of a source to an ICRS geometric position, by
 * undoing the gravitational deflection and aberration corrections.
 *
 * @param frame           The observer frame, defining the location and time of observation
 * @param sys             The reference system in which the observed position is specified.
 * @param ra              [h] Observed ICRS right-ascension of the source
 * @param dec             [deg] Observed ICRS declination of the source
 * @param dist            [AU] Observed distance from observer. A value of &lt;=0 will translate
 *                        to 10<sup>15</sup> AU (around 5 Gpc).
 * @param[out] geom_icrs  [AU] The corresponding geometric position for the source, in ICRS.
 * @return              0 if successful, or else an error from grav_undef2(), or -1 (errno will
 *                      indicate the type of error).
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa novas_geom_to_app()
 * @sa novas_hor_to_app(), novas_geom_to_hor(), novas_transform_vector()
 */
int novas_app_to_geom(const novas_frame *restrict frame, enum novas_reference_system sys, double ra, double dec,
        double dist, double *restrict geom_icrs) {
  static const char *fn = "novas_apparent_to_nominal";
  double app_pos[3];

  if(!frame || !geom_icrs)
    return novas_error(-1, EINVAL, fn, "NULL argument: frame=%p, geom_icrs=%p", frame, geom_icrs);

  if(!novas_frame_is_initialized(frame))
    return novas_error(-1, EINVAL, fn, "frame at %p not initialized", frame);

  if(sys < 0 || sys >= NOVAS_REFERENCE_SYSTEMS)
    return novas_error(-1, EINVAL, fn, "invalid reference system: %d", sys);

  if(dist <= 0.0) dist = 1e15;

  // 3D apparent position
  radec2vector(ra, dec, dist, app_pos);

  // Convert apparent position to ICRS...
  switch(sys) {
    case NOVAS_ITRS:
      // ITRS -> TIRS
      wobble(frame->time.ijd_tt + frame->time.fjd_tt, WOBBLE_ITRS_TO_TIRS, frame->dx, frame->dy, app_pos, app_pos); // @suppress("No break at end of case")
      /* fallthrough */
    case NOVAS_TIRS:
      // TIRS -> CIRS
      spin(-frame->era, app_pos, app_pos); // @suppress("No break at end of case")
      /* fallthrough */
    case NOVAS_CIRS:
      matrix_inv_rotate(app_pos, &frame->gcrs_to_cirs, app_pos);
      break;

    case NOVAS_TOD:
      matrix_inv_rotate(app_pos, &frame->nutation, app_pos); // @suppress("No break at end of case")
      /* fallthrough */
    case NOVAS_MOD:
      matrix_inv_rotate(app_pos, &frame->precession, app_pos); // @suppress("No break at end of case")
      /* fallthrough */
    case NOVAS_J2000:
      matrix_inv_rotate(app_pos, &frame->icrs_to_j2000, app_pos);
      break;
    default:
      // nothing to do.
      ;
  }

  // Undo aberration correction
  frame_aberration(frame, APP_TO_GEOM, app_pos);

  // Undo gravitational deflection and aberration.
  prop_error(fn, grav_undo_planets(app_pos, frame->obs_pos, &frame->planets, geom_icrs), 0);

  return 0;
}

/**
 * Calculates a transformation matrix that can be used to convert positions and velocities from
 * one coordinate reference system to another.
 *
 * @param frame           Observer frame, defining the location and time of observation
 * @param from_system     Original coordinate reference system
 * @param to_system       New coordinate reference system
 * @param[out] transform  Pointer to the transform data structure to populate.
 * @return                0 if successful, or else -1 if there was an error (errno will indicate
 *                        the type of error).
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa novas_transform_vector(), novas_transform_sky_pos(), novas_invert_transform()
 * @sa novas_geom_posvel(), novas_app_to_geom()
 */
int novas_make_transform(const novas_frame *frame, enum novas_reference_system from_system, enum novas_reference_system to_system,
        novas_transform *transform) {
  static const char *fn = "novas_calc_transform";
  int i, dir;

  if(!frame || !transform)
    return novas_error(-1, EINVAL, fn, "NULL argument: frame=%p, transform=%p", frame, transform);

  if(!novas_frame_is_initialized(frame))
    return novas_error(-1, EINVAL, fn, "frame at %p not initialized", frame);

  if(to_system < 0 || to_system >= NOVAS_REFERENCE_SYSTEMS)
    return novas_error(-1, EINVAL, fn, "invalid reference system (to): %d\n", to_system);

  transform->frame = *frame;
  transform->from_system = from_system;
  transform->to_system = to_system;

  memset(&transform->matrix, 0, sizeof(transform->matrix));

  // Identity matrix
  for(i = 3; --i >= 0;)
    transform->matrix.M[i][i] = 1.0;

  // We treat ICRS and GCRS as the same here
  // The aberration and gravitational bending are accounted for in
  // novas_calc_apparent
  if(from_system == NOVAS_ICRS)
    from_system = NOVAS_GCRS;
  if(to_system == NOVAS_ICRS)
    to_system = NOVAS_GCRS;

  dir = cmp_sys(to_system, from_system);

  if(dir == 0)
    return 0;

  if(dir < 0) {
    switch(from_system) {
      case NOVAS_ITRS: {
        novas_matrix W;
        set_wobble_matrix(frame, &W);
        add_transform(transform, &W, -1);
        if(to_system == NOVAS_TIRS)
          return 0;
      } // @suppress("No break at end of case")
      /* fallthrough */

      case NOVAS_TIRS: {
        novas_matrix R;
        set_spin_matrix(frame->era, &R);
        add_transform(transform, &R, -1);
        if(to_system == NOVAS_CIRS)
          return 0;
      } // @suppress("No break at end of case")
      /* fallthrough */

      case NOVAS_CIRS:
        add_transform(transform, &frame->gcrs_to_cirs, -1);
        if(to_system == NOVAS_GCRS)
          return 0; // @suppress("No break at end of case")
        /* fallthrough */

      case NOVAS_GCRS:
        add_transform(transform, &frame->icrs_to_j2000, 1);
        if(to_system == NOVAS_J2000)
          return 0; // @suppress("No break at end of case")
        /* fallthrough */

      case NOVAS_J2000:
        add_transform(transform, &frame->precession, 1);
        if(to_system == NOVAS_MOD)
          return 0; // @suppress("No break at end of case")
        /* fallthrough */

      case NOVAS_MOD:
        add_transform(transform, &frame->nutation, 1);
        return 0;

      default:
        // nothing to do...
        ;
    }
  }
  else {
    switch(from_system) {
      case NOVAS_TOD:
        add_transform(transform, &frame->nutation, -1);
        if(to_system == NOVAS_MOD)
          return 0; // @suppress("No break at end of case")
        /* fallthrough */

      case NOVAS_MOD:
        add_transform(transform, &frame->precession, -1);
        if(to_system == NOVAS_J2000)
          return 0; // @suppress("No break at end of case")
        /* fallthrough */

      case NOVAS_J2000:
        add_transform(transform, &frame->icrs_to_j2000, -1);
        if(to_system == NOVAS_GCRS)
          return 0; // @suppress("No break at end of case")
        /* fallthrough */

      case NOVAS_GCRS:
        add_transform(transform, &frame->gcrs_to_cirs, 1);
        if(to_system == NOVAS_CIRS)
          return 0; // @suppress("No break at end of case")
        /* fallthrough */

      case NOVAS_CIRS: {
        novas_matrix R;
        set_spin_matrix(frame->era, &R);
        add_transform(transform, &R, 1);
        if(to_system == NOVAS_TIRS)
          return 0;
        } // @suppress("No break at end of case")
        /* fallthrough */

      case NOVAS_TIRS: {
        novas_matrix W;
        set_wobble_matrix(frame, &W);
        add_transform(transform, &W, 1);
        return 0;
      }

      default:
        // nothing to do...
        ;
    }
  }

  return novas_error(-1, EINVAL, fn, "invalid reference system (from): %d\n", from_system);
}

/**
 * Inverts a novas coordinate transformation matrix.
 *
 * @param transform     Pointer to a coordinate transformation matrix.
 * @param[out] inverse  Pointer to a coordinate transformation matrix to populate with the inverse
 *                      transform. It may be the same as the input.
 * @return              0 if successful, or else -1 if the was an error (errno will indicate the
 *                      type of error).
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa novas_make_transform()
 */
int novas_invert_transform(const novas_transform *transform, novas_transform *inverse) {
  novas_transform orig;

  if(!transform || !inverse)
    return novas_error(-1, EINVAL, "novas_invert_transform", "NULL argument: transform=%p, inverse=%p", transform, inverse);

  orig = *transform;
  *inverse = orig;
  invert_matrix(&orig.matrix, &inverse->matrix);

  return 0;
}

/**
 * Transforms a position or velocity 3-vector from one coordinate reference system to another.
 *
 * @param in          Input 3-vector in the original coordinate reference system
 * @param transform   Pointer to a coordinate transformation matrix
 * @param[out] out    Output 3-vector in the new coordinate reference system. It may be the same
 *                    as the input.
 * @return            0 if successful, or else -1 if there was an error (errno will indicate the
 *                    type of error).
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa novas_make_transform(), novas_transform_skypos()
 */
int novas_transform_vector(const double *in, const novas_transform *restrict transform, double *out) {
  static const char *fn = "novas_matrix_transform";

  if(!transform || !in  || !out)
    return novas_error(-1, EINVAL, fn, "NULL parameter: in=%p, transform=%p, out=%p", in, transform, out);

  prop_error(fn, matrix_transform(in, &transform->matrix, out), 0);

  return 0;
}

/**
 * Transforms a position or velocity 3-vector from one coordinate reference system to another.
 *
 * @param in          Input apparent position on sky in the original coordinate reference system
 * @param transform   Pointer to a coordinate transformation matrix
 * @param[out] out    Output apparent position on sky in the new coordinate reference system.
 *                    It may be the same as the input.
 * @return            0 if successful, or else -1 if there was an error (errno will indicate the
 *                    type of error).
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa novas_sky_pos(), novas_make_transform(), novas_transform_vector()
 */
int novas_transform_sky_pos(const sky_pos *in, const novas_transform *restrict transform, sky_pos *out) {
  static const char *fn = "novas_matrix_transform";

  if(!transform || !in  || !out)
    return novas_error(-1, EINVAL, fn, "NULL parameter: in=%p, transform=%p, out=%p", in, transform, out);

  prop_error(fn, matrix_transform(in->r_hat, &transform->matrix, out->r_hat), 0);
  vector2radec(out->r_hat, &out->ra, &out->dec);

  return 0;
}

/**
 * Returns the Local (apparent) Sidereal Time for an observing frame of an Earth-bound observer.
 *
 * @param frame   Observer frame, defining the location and time of observation
 * @return        [h] The LST for an Earth-bound observer [0.0--24.0), or NAN otherwise. If NAN is
 *                returned errno will indicate the type of error.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_time_lst()
 */
double novas_frame_lst(const novas_frame *restrict frame) {
  static const char *fn = "novas_frame_lst";
  double lst;

  if(!frame) {
    novas_set_errno(EINVAL, fn, "input frame is NULL");
    return NAN;
  }

  if(!novas_frame_is_initialized(frame)) {
    novas_set_errno(EINVAL, fn, "input frame is not initialized");
    return NAN;
  }

  if(frame->observer.where != NOVAS_OBSERVER_ON_EARTH && frame->observer.where != NOVAS_AIRBORNE_OBSERVER) {
    novas_set_errno(EINVAL, fn, "Not an Earth-bound observer: where=%d", frame->observer.where);
    return NAN;
  }

  lst = remainder(frame->gst + frame->observer.on_surf.longitude / 15.0, DAY_HOURS);
  if(lst < 0.0)
    lst += DAY_HOURS;

  return lst;
}

/**
 * Returns the hourangle at which the object crosses the specified elevation angle.
 *
 *
 * @param el          [rad] Elevation angle.
 * @param dec         [rad] Apparent Source declination.
 * @param lat         [rad] Geodetic latitude of observer.
 * @return            [h] the hour angle at which the source crosses the specified elevation
 *                    angle, or else NAN if the source stays above or below the specified
 *                    elevation at all times.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_sets_below()
 */
static double calc_lha(double el, double dec, double lat) {
  double c = (sin(el) - sin(lat) * sin(dec)) / (cos(lat) * cos(dec));
  return acos(c) / NOVAS_HOURANGLE;
}

/**
 * Returns the UTC date at which a distant source appears cross the specified elevation angle, or
 * when it transits the local meridian. The calculated time will account for the (slow) motion of
 * the source (for Solar-system objects), and optionally for atmospheric refraction also.
 *
 * NOTES:
 * <ol>
 * <li>The current implementation is not suitable for calculating nearest successive rise/set
 * times for near-Earth objects, at or within the geostationary orbit.</li>
 * </ol>
 *
 * @param el          [deg] Elevation angle (not used for transit times).
 * @param sign        1 for rise time, or -1 for setting time, or 0 for transit.
 * @param source      Observed source
 * @param frame       Observing frame, defining the observer location and astronomical time of
 *                    observation, which defines a lower-bound for the returned time.
 * @param ref_model   Refraction model, or NULL to calculate unrefracted rise/set times.
 * @return            [day] UTC-based Julian date at which the object crosses the specified
 *                    elevation (or transits) in the 24 hour period after the specified date, or
 *                    else NAN if the source stays above or below the given elevation for the
 *                    entire 24-hour period. Valid times have a typical accuracy at the
 *                    millisecond level.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 */
static double novas_cross_el_date(double el, int sign, const object *source, const novas_frame *frame, RefractionModel ref_model) {
  static const char *fn = "novas_cross_el_date";

  const on_surface *loc;
  novas_frame frame1;
  double jd0_tt;
  int i;

  if(!source) {
    novas_set_errno(EINVAL, fn, "input source is NULL");
    return NAN;
  }

  if(isnan(novas_frame_lst(frame)))
    return novas_trace_nan(fn);

  el *= DEGREE;                     // convert to degrees.
  frame1 = *frame;                  // Time shifted frame
  jd0_tt = novas_get_time(&frame->time, NOVAS_TT);
  loc = (on_surface *) &frame->observer.on_surf;   // Earth-bound location

  for(i = 0; i < novas_inv_max_iter; i++) {
    sky_pos pos = SKY_POS_INIT;
    novas_timespec t = frame1.time;
    double ref = 0.0, lha, dhr;

    prop_error(fn, novas_sky_pos(source, &frame1, NOVAS_TOD, &pos), 0);

    if(ref_model)
      // Apply (possibly time-specific) refraction correction
      ref = ref_model(novas_get_time(&t, NOVAS_TT), &frame->observer.on_surf, NOVAS_REFRACT_OBSERVED, el) * DEGREE;

    // Hourangle when source crosses nominal elevation
    lha = sign ? calc_lha(el - ref, pos.dec * NOVAS_DEGREE, loc->latitude * NOVAS_DEGREE) : 0.0;
    if(isnan(lha)) {
      errno = 0;      // It's to be expected for some sources.
      return NAN;
    }

    // Adjusted frame time for last crossing time estimate
    dhr = remainder((pos.ra + sign * lha - novas_frame_lst(&frame1)), DAY_HOURS);
    t.fjd_tt += dhr / DAY_HOURS / SIDEREAL_RATE;

    // Make sure that calculated time is after input frame time.
    if((t.ijd_tt + t.fjd_tt) < jd0_tt) {
      t.ijd_tt++;
      dhr += DAY_HOURS / SIDEREAL_RATE;
    }

    // Done if catalog source or if time converged to ms accuracy
    // We run into the numerical precision limit for the Moon at around 1e-8 hours, so
    // the convergence criterion should be significantly higher than that.
    if(source->type == NOVAS_CATALOG_OBJECT || fabs(dhr) < 1e-7)
      return novas_get_time(&t, NOVAS_UTC);

    // Make a new observer frame for the shifted time for the next iteration
    novas_make_frame(frame->accuracy, &frame->observer, &t, frame->dx, frame->dy, &frame1);
  }

  novas_set_errno(ECANCELED, fn, "failed to converge");
  return NAN;
}

/**
 * Returns the UTC date at which a source transits the local meridian. The calculated time will
 * account for the (slow) motion of Solar-system bodies.
 *
 * NOTES:
 * <ol>
 * <li>The current implementation is not suitable for calculating the nearest successive transit
 * times for near-Earth objects, at or within the geostationary orbit.</li>
 * </ol>
 *
 * @param source      Observed source
 * @param frame       Observing frame, defining the observer location and astronomical time of
 *                    observation.
 * @return            [day] UTC-based Julian date at which the object transits the local meridian
 *                    next after the specified date, or NAN if either input pointer is NULL.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_rises_above(), novas_sets_below()
 */
double novas_transit_time(const object *restrict source, const novas_frame *restrict frame) {
  double utc = novas_cross_el_date(NAN, 0, source, frame, NULL);
  if(isnan(utc))
    return novas_trace_nan("novas_rises_above");
  return utc;
}

/**
 * Returns the UTC date at which a distant source appears to rise above the specified elevation
 * angle. The calculated time will account for the (slow) motion for Solar-system bodies, and
 * optionally for atmospheric refraction also.
 *
 * NOTES:
 * <ol>
 * <li>The current implementation is not suitable for calculating the nearest successive rise
 * times for near-Earth objects, at or within the geostationary orbit.</li>
 * <li>This function calculates the time when the center (not the limb!) of the source rises
 * above the specified elevation threshold. Something to keep in mind for calculating Sun/Moon
 * rise times.</li>
 * </ol>
 *
 * @param el          [deg] Elevation angle.
 * @param source      Observed source
 * @param frame       Observing frame, defining the observer location and astronomical time of
 *                    observation.
 * @param ref_model   Refraction model, or NULL to calculate unrefracted rise time.
 * @return            [day] UTC-based Julian date at which the object rises above the specified
 *                    elevation next after the specified date, or else NAN if the source stays
 *                    above or below the given elevation for the entire 24-hour period.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_sets_below(), novas_transit_time()
 */
double novas_rises_above(double el, const object *restrict source, const novas_frame *restrict frame, RefractionModel ref_model) {
  double utc;

  errno = 0;

  utc = novas_cross_el_date(el, -1, source, frame, ref_model);
  if(isnan(utc) && errno)
    return novas_trace_nan("novas_rises_above");

  return utc;
}

/**
 * Returns the UTC date at which a distant source appears to set below the specified elevation
 * angle. The calculated time will account for the (slow) motion of Solar-system bodies, and
 * optionally for atmopsheric refraction also.
 *
 * NOTES:
 * <ol>
 * <li>The current implementation is not suitable for calculating the nearest successive set
 * times for near-Earth objects, at or within the geostationary orbit.</li>
 * <li>This function calculates the time when the center (not the limb!) of the source sets below
 * the specified elevation threshold. Something to keep in mind for calculating Sun/Moon rise
 * times.</li>
 * </ol>
 *
 * @param el          [deg] Elevation angle.
 * @param source      Observed source
 * @param frame       Observing frame, defining the observer location and astronomical time of
 *                    observation.
 * @param ref_model   Refraction model, or NULL to calculate unrefracted setting time.
 * @return            [day] UTC-based Julian date at which the object sets below the specified
 *                    elevation next after the specified date, or else NAN if the source stays
 *                    above or below the given elevation for the entire 24-hour day..
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_rises_above(), novas_transit_time()
 */
double novas_sets_below(double el, const object *restrict source, const novas_frame *restrict frame, RefractionModel ref_model) {
  double utc;

  errno = 0;

  utc = novas_cross_el_date(el, 1, source, frame, ref_model);
  if(isnan(utc) && errno)
    return novas_trace_nan("novas_sets_below");

  return utc;
}

/**
 * Returns the Solar illumination fraction of a source, assuming a spherical geometry for the
 * observed body.
 *
 * @param source    Observed source. Usually a Solar-system source. (For other source types, 1.0
 *                  is returned by default.)
 * @param frame     Observing frame, defining the observer location and astronomical time of
 *                  observation.
 * @return          Solar illumination fraction [0.0:1.0] of a spherical body observed at the
 *                  source location from the given observer location, or NAN if there was an error
 *                  (errno will indicate the type of error).
 *
 * @since 1.3
 * @author Attila Kovacs
 */
double novas_solar_illum(const object *restrict source, const novas_frame *restrict frame) {
  static const char *fn = "novas_solar_illum";

  double pos[3], dSrc, dObs, dSun;
  int i;

  if(!source) {
    novas_set_errno(EINVAL, fn, "input source is NULL");
    return NAN;
  }

  if(!frame) {
    novas_set_errno(EINVAL, fn, "input frame is NULL");
    return NAN;
  }

  if(!novas_frame_is_initialized(frame)) {
    novas_set_errno(EINVAL, fn, "input frame is not initialized");
    return NAN;
  }

  if(source->type == NOVAS_CATALOG_OBJECT)
    return 1.0;

  if(novas_geom_posvel(source, frame, NOVAS_ICRS, pos, NULL) != 0)
    return novas_trace_nan(fn);

  dSrc = novas_vlen(pos);

  for(i = 3; --i >= 0; )
    pos[i] += frame->obs_pos[i];

  dSun = novas_vdist(pos, frame->sun_pos);
  dObs = novas_vdist(frame->obs_pos, frame->sun_pos);

  return 0.5 + 0.5 * (dSrc * dSrc + dSun * dSun - dObs * dObs) / (2.0 * dSun * dSrc);
}

/**
 * Returns the angular separation of two objects from the observer's point of view. The
 * calculated separation includes light-time corrections, aberration and gravitational
 * deflection for both sources, and thus represents a precise observed separation between the two
 * sources.
 *
 * @param source1   An observed source
 * @param source2   Another observed source
 * @param frame     Observing frame, defining the observer location and astronomical time of
 *                  observation.
 * @return          [deg] Apparent angular separation between the two observed source from the
 *                  observer's point-of-view.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_sep(), novas_sun_angle(), novas_moon_angle()
 */
double novas_object_sep(const object *source1, const object *source2, const novas_frame *restrict frame) {
  static const char *fn = "novas_object_sep";

  sky_pos p1 = SKY_POS_INIT, p2 = SKY_POS_INIT;

  if(!source1) {
    novas_set_errno(EINVAL, fn, "input source1 is NULL");
    return NAN;
  }

  if(!source2) {
    novas_set_errno(EINVAL, fn, "input source2 is NULL");
    return NAN;
  }

  if(novas_sky_pos(source1, frame, NOVAS_GCRS, &p1) != 0)
    return novas_trace_nan(fn);

  if(novas_sky_pos(source2, frame, NOVAS_GCRS, &p2) != 0)
    return novas_trace_nan(fn);

  if(p1.dis < 1e-11 || p2.dis < 1e-11) {
    novas_set_errno(EINVAL, fn, "source is at observer location");
    return NAN;
  }

  return novas_equ_sep(p1.ra, p1.dec, p2.ra, p2.dec);
}

/**
 * Returns the apparent angular distance of a source from the Sun from the observer's point of
 * view.
 *
 * @param source    An observed source
 * @param frame     Observing frame, defining the observer location and astronomical time of
 *                  observation.
 * @return          [deg] the apparent angular distance between the source an the Sun, from the
 *                  observer's point of view.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_moon_angle()
 */
double novas_sun_angle(const object *restrict source, const novas_frame *restrict frame) {
  object sun = NOVAS_SUN_INIT;
  double d = novas_object_sep(source, &sun, frame);
  if(isnan(d))
    return novas_trace_nan("novas_sun_angle");
  return d;
}

/**
 * Returns the apparent angular distance of a source from the Moon from the observer's point of
 * view.
 *
 * @param source    An observed source
 * @param frame     Observing frame, defining the observer location and astronomical time of
 *                  observation.
 * @return          [deg] Apparent angular distance between the source an the Moon, from the
 *                  observer's point of view.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_sun_angle()
 */
double novas_moon_angle(const object *restrict source, const novas_frame *restrict frame) {
  object moon = NOVAS_MOON_INIT;
  double d = novas_object_sep(source, &moon, frame);
  if(isnan(d))
    return novas_trace_nan("novas_moon_angle");
  return d;
}

/// \cond PRIVATE
double novas_unwrap_angles(double *a, double *b, double *c) {
  // Careful with Az wraps
  if(*a >= 270.0 || *b >= 270.0 || *c >= 270.0) {
    if(*a < 90.0)
      *a += DEG360;
    if(*b < 90.0)
      *b += DEG360;
    if(*c < 90.0)
      *c += DEG360;
  }
  return 0;
}
/// \endcond

/**
 * Calculates equatorial tracking position and motion (first and second time derivatives) for the
 * specified source in the given observing frame. The position and its derivatives are calculated
 * via the more precise IAU2006 method, and CIRS.
 *
 * @param source        Observed source
 * @param frame         Observing frame, defining the observer location and astronomical time of
 *                      observation.
 * @param dt            [s] Time step used for calculating derivatives.
 * @param[out] track    Output tracking parameters to populate
 * @return              0 if successful, or else -1 if any of the pointer arguments are NULL, or
 *                      else an error code from novas_sky_pos().
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_hor_track(), novas_track_pos()
 */
int novas_equ_track(const object *restrict source, const novas_frame *restrict frame, double dt, novas_track *restrict track) {
  static const char *fn = "novas_equ_track";

  novas_timespec time1;
  novas_frame frame1;
  sky_pos pos0 = SKY_POS_INIT, posm = SKY_POS_INIT, posp = SKY_POS_INIT;
  double ra_cio;
  double idt2;

  if(dt <= 0.0) dt = NOVAS_TRACK_DELTA;
  idt2 = 1.0 / (dt * dt);

  if(!source)
    return novas_error(-1, EINVAL, fn, "input source is NULL");

  if(!frame)
    return novas_error(-1, EINVAL, fn, "input frame is NULL");

  if(!novas_frame_is_initialized(frame))
    return novas_error(-1, EINVAL, fn, "input frame is not initialized");

  if(!track)
    return novas_error(-1, EINVAL, fn, "output track is NULL");

  track->time = frame->time;

  ra_cio = -ira_equinox(frame->time.ijd_tt + frame->time.fjd_tt, NOVAS_TRUE_EQUINOX, frame->accuracy);
  prop_error(fn, novas_sky_pos(source, frame, NOVAS_CIRS, &pos0), 0);
  pos0.ra += ra_cio;
  pos0.rv = novas_v2z(pos0.rv);

  track->pos.lon = 15.0 * pos0.ra;
  track->pos.lat = pos0.dec;
  track->pos.dist = pos0.dis;
  track->pos.z = pos0.rv;

  time1 = frame->time;
  time1.fjd_tt -= dt / DAY;
  prop_error(fn, novas_make_frame(frame->accuracy, &frame->observer, &time1, frame->dx, frame->dy, &frame1), 0);
  prop_error(fn, novas_sky_pos(source, &frame1, NOVAS_CIRS, &posm), 0);
  posm.ra += ra_cio;
  posm.rv = novas_v2z(posm.rv);

  time1.fjd_tt += 2.0 * dt / DAY;
  prop_error(fn, novas_make_frame(frame->accuracy, &frame->observer, &time1, frame->dx, frame->dy, &frame1), 0);
  prop_error(fn, novas_sky_pos(source, &frame1, NOVAS_CIRS, &posp), 0);
  posp.ra += ra_cio;
  posp.rv = novas_v2z(posp.rv);

  // hours -> degrees...
  pos0.ra *= 15.0;
  posm.ra *= 15.0;
  posp.ra *= 15.0;

  // Careful with RA wraps.
  novas_unwrap_angles(&posm.ra, &pos0.ra, &posp.ra);

  track->rate.lon = 0.5 * (posp.ra - posm.ra) / dt;
  track->rate.lat = 0.5 * (posp.dec - posm.dec) / dt;
  track->rate.dist = 0.5 * (posp.dis - posm.dis) / dt;
  track->rate.z = 0.5 * (posp.rv - posm.rv) / dt;

  track->accel.lon = (0.5 * (posp.ra + posm.ra) - pos0.ra) * idt2;
  track->accel.lat = (0.5 * (posp.dec + posm.dec) - pos0.dec) * idt2;
  track->accel.dist = (0.5 * (posp.dis + posm.dis) - pos0.dis) * idt2;
  track->accel.z = (0.5 * (posp.rv + posm.rv) - pos0.rv) * idt2;

  return 0;
}

/**
 * Calculates horizontal tracking position and motion (first and second time derivatives) for the
 * specified source in the given observing frame. The position and its derivatives are calculated
 * via the more precise IAU2006 method, and CIRS, and then converted to local horizontal
 * coordinates using the specified refraction model (if any).
 *
 * @param source        Observed source
 * @param frame         Observing frame, defining the observer location and astronomical time of
 *                      observation.
 * @param ref_model     Refraction model to use, or NULL for an unrefracted track.
 * @param[out] track    Output tracking parameters to populate
 * @return              0 if successful, or else -1 if any of the pointer arguments are NULL, or
 *                      else an error code from novas_sky_pos(), or from novas_app_hor().
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_equ_track(), novas_track_pos()
 */
int novas_hor_track(const object *restrict source, const novas_frame *restrict frame, RefractionModel ref_model,
        novas_track *restrict track) {
  static const char *fn = "novas_equ_track";

  novas_timespec time1;
  novas_frame frame1;
  sky_pos pos = SKY_POS_INIT;
  double ra_cio;
  double az0, el0, azm, elm, dm, zm, azp, elp, dp, zp;
  const double idt2 = 1.0 / (NOVAS_TRACK_DELTA * NOVAS_TRACK_DELTA);

  if(!source)
    return novas_error(-1, EINVAL, fn, "input source is NULL");

  if(!frame)
    return novas_error(-1, EINVAL, fn, "input frame is NULL");

  if(!novas_frame_is_initialized(frame))
    return novas_error(-1, EINVAL, fn, "input frame is not initialized");

  if(frame->observer.where != NOVAS_OBSERVER_ON_EARTH && frame->observer.where != NOVAS_AIRBORNE_OBSERVER)
    return novas_error(-1, EINVAL, fn, "observer is not Earth-bound: where = %d", frame->observer.where);

  if(!track)
    return novas_error(-1, EINVAL, fn, "output track is NULL");

  track->time = frame->time;
  ra_cio = -ira_equinox(frame->time.ijd_tt + frame->time.fjd_tt, NOVAS_TRUE_EQUINOX, frame->accuracy);

  prop_error(fn, novas_sky_pos(source, frame, NOVAS_CIRS, &pos), 0);
  prop_error(fn, novas_app_to_hor(frame, NOVAS_TOD, pos.ra + ra_cio, pos.dec, ref_model, &az0, &el0), 0);
  track->pos.lon = az0;
  track->pos.lat = el0;
  track->pos.dist = pos.dis;
  track->pos.z = novas_v2z(pos.rv);

  time1 = frame->time;
  time1.fjd_tt -= NOVAS_TRACK_DELTA / DAY;
  prop_error(fn, novas_make_frame(frame->accuracy, &frame->observer, &time1, frame->dx, frame->dy, &frame1), 0);
  prop_error(fn, novas_sky_pos(source, &frame1, NOVAS_CIRS, &pos), 0);
  prop_error(fn, novas_app_to_hor(&frame1, NOVAS_TOD, pos.ra + ra_cio, pos.dec, ref_model, &azm, &elm), 0);
  dp = pos.dis;
  zm = novas_v2z(pos.rv);

  time1.fjd_tt += 2.0 * NOVAS_TRACK_DELTA / DAY;
  prop_error(fn, novas_make_frame(frame->accuracy, &frame->observer, &time1, frame->dx, frame->dy, &frame1), 0);
  prop_error(fn, novas_sky_pos(source, &frame1, NOVAS_CIRS, &pos), 0);
  prop_error(fn, novas_app_to_hor(&frame1, NOVAS_TOD, pos.ra + ra_cio, pos.dec, ref_model, &azp, &elp), 0);
  dm = pos.dis;
  zp = novas_v2z(pos.rv);

  // Careful with Az wraps
  novas_unwrap_angles(&azm, &az0, &azp);

  track->rate.lon = 0.5 * (azp - azm) / NOVAS_TRACK_DELTA;
  track->rate.lat = 0.5 * (elp - elm) / NOVAS_TRACK_DELTA;
  track->rate.dist = 0.5 * (dp - dm) / NOVAS_TRACK_DELTA;
  track->rate.z = 0.5 * (zp - zm) / NOVAS_TRACK_DELTA;

  track->accel.lon = (0.5 * (azp + azm) - az0) * idt2;
  track->accel.lat = (0.5 * (elp + elm) - el0) * idt2;
  track->accel.dist = (0.5 * (dp + dm) - track->pos.dist) * idt2;
  track->accel.z = (0.5 * (zp + zm) - track->pos.z) * idt2;

  return 0;
}

/**
 * Calculates a projected position and redshift for a source, given the available tracking
 * position and derivatives. Using 'tracks' to project positions can be much faster than the
 * repeated full recalculation of the source position over some short period.
 *
 * In SuperNOVAS terminology a 'track' is a 2nd order Taylor series expansion of the observed
 * position and redshift in time. For most but the fastest moving sources, horizontal (Az/El)
 * tracks are sufficiently precise on minute timescales, whereas depending on the type of source
 * equatorial tracks can be precise for up to days.
 *
 * @param track       Tracking position and motion (first and second derivatives)
 * @param time        Astrometric time of observation
 * @param[out] lon    [deg] projected observed Eastward longitude in tracking coordinate system
 * @param[out] lat    [deg] projected observed latitude in tracking coordinate system
 * @param[out] dist   [AU] projected apparent distance to source from observer
 * @param[out] z      projected observed redshift
 * @return            0 if successful, or else -1 if either input pointer is NULL (errno is set to
 *                    EINVAL).
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_equ_track(), novas_hor_track()
 * @sa novas_z2v()
 */
int novas_track_pos(const novas_track *track, const novas_timespec *time, double *restrict lon, double *restrict lat,
        double *restrict dist, double *restrict z) {
  static const char *fn = "novas_track_pos";

  double dt, dt2;

  if(!time)
    return novas_error(-1, EINVAL, fn, "input time is NULL");

  if(!track)
    return novas_error(-1, EINVAL, fn, "input track is NULL");

  dt = novas_diff_time(time, &track->time);
  dt2 = dt * dt;

  if(lon)
    *lon = remainder(track->pos.lon + track->rate.lon * dt + track->accel.lon * dt2, DEG360);
  if(lat)
    *lat = track->pos.lat + track->rate.lat * dt + track->accel.lat * dt2;
  if(dist)
    *dist = track->pos.dist + track->rate.dist * dt + track->accel.dist * dt2;
  if(z)
    *z = track->pos.z + track->rate.z * dt + track->accel.z * dt2;

  return 0;
}

/**
 * @file
 *
 * @author G. Kaplan and A. Kovacs
 *
 *  SuperNOVAS planetary ephemeris manager for the planet_eph_manager and planet_eph_manager_hp()
 *  functions.
 *
 *  This module exposes a lot of its own internal state variables globally. You probably should
 *  not access them from outside this module, but they are kept ad globals to ensure compatibility
 *  with existing NOVAS C applications that might access those values.
 *
 *  Based on the NOVAS C Edition, Version 3.1:
 *
 *  U. S. Naval Observatory<br>
 *  Astronomical Applications Dept.<br>
 *  Washington, DC<br>
 *  <a href="http://www.usno.navy.mil/USNO/astronomical-applications">
 *  http://www.usno.navy.mil/USNO/astronomical-applications</a>
 *
 *  @sa solsys2.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#include "eph_manager.h"

/// \cond PRIVATE
#define __NOVAS_INTERNAL_API__      ///< Use definitions meant for internal use by SuperNOVAS only

#define EM1   (EM_RATIO + 1.0)      ///< for local use...
/// \endcond

#include "novas.h"

/// Flag that defines physical units of the output states. 1: km and km/sec; 0: AU and AU/day.
/// Its default value is 0 (KM determines time unit for nutations. Angle unit is always radians.)
short KM;           ///< Flag that defines physical units of the output states.

// IPT and LPT defined as int to support 64 bit systems.
int IPT[3][12];     ///< (<i>for internal use</i>)
int LPT[3];         ///< (<i>for internal use</i>)

long NRL;           ///< (<i>for internal use</i>) Current record number buffered
long NP;            ///< (<i>for internal use</i>)
long NV;            ///< (<i>for internal use</i>)
long RECORD_LENGTH; ///< (<i>for internal use</i>)

double SS[3];       ///< (<i>for internal use</i>)
double JPLAU;       ///< (<i>for internal use</i>)
double PC[18];      ///< (<i>for internal use</i>)
double VC[18];      ///< (<i>for internal use</i>)
double TWOT;        ///< (<i>for internal use</i>)
double EM_RATIO;    ///< (<i>for internal use</i>)
double *BUFFER;     ///< (<i>for internal use</i>) Array containing Chebyshev coefficients of position.


FILE *EPHFILE = NULL;     ///< (<i>for internal use</i>) The currently open JPL DE planetary ephemeris file

/**
 * This function opens a JPL planetary ephemeris file and sets initial values.  This function must
 * be called prior to calls to the other JPL ephemeris functions.
 *
 * REFERENCES:
 * <ol>
 * <li>Standish, E.M. and Newhall, X X (1988). "The JPL Export Planetary Ephemeris"; JPL document
 * dated 17 June 1988.</li>
 * </ol>
 *
 * @param ephem_name      Name/path of the direct-access ephemeris file.
 * @param[out] jd_begin   [day] Beginning Julian date of the ephemeris file. It may be NULL if not
 *                        required.
 * @param[out] jd_end     [day] Ending Julian date of the ephemeris file. It may be NULL if not
 *                        required.
 * @param[out] de_number  DE number of the ephemeris file opened. It may be NULL if not required.
 *
 * @return    0 if successful, or -1 if any of the arhuments is NULL, or else 1 if the file could
 *            not be opened, 2--10 if (= line + 1) if there was an error reading the header line,
 *            or 11 if the type of DE file is not supported.
 *
 * @sa ephem_close()
 */
short ephem_open(const char *ephem_name, double *jd_begin, double *jd_end, short *de_number) {
  static const char *fn = "ephem_open";
  int ncon, denum;

  if(!ephem_name)
    return novas_error(-1, EINVAL, fn, "NULL input file name/path");

  if(EPHFILE) {
    fclose(EPHFILE);
    free(BUFFER);
  }

  // Open file ephem_name.
  if((EPHFILE = fopen(ephem_name, "rb")) == NULL) {
    return novas_error(1, errno, fn, "cannot open '%s': %s", ephem_name, strerror(errno));
  }
  else {
    char ttl[252], cnam[2400];
    short i, j;

    // File found. Set initializations and default values.

    KM = 0;

    NRL = 0;

    NP = 2;
    NV = 3;
    TWOT = 0.0;

    for(i = 0; i < 18; i++) {
      PC[i] = 0.0;
      VC[i] = 0.0;
    }

    PC[0] = 1.0;
    VC[1] = 1.0;

    // Read in values from the first record, aka the header.

    if(fread(ttl, sizeof ttl, 1, EPHFILE) != 1) {
      fclose(EPHFILE);
      return novas_error(2, errno, fn, "reading 'ttl' from '%s': %s", ephem_name, strerror(errno));
    }
    if(fread(cnam, sizeof cnam, 1, EPHFILE) != 1) {
      fclose(EPHFILE);
      return novas_error(4, errno, fn, "reading 'cnam' from '%s': %s", ephem_name, strerror(errno));
    }
    if(fread(SS, sizeof SS, 1, EPHFILE) != 1) {
      fclose(EPHFILE);
      return novas_error(4, errno, fn, "reading 'SS' from '%s': %s", ephem_name, strerror(errno));
    }
    if(fread(&ncon, sizeof ncon, 1, EPHFILE) != 1) {
      fclose(EPHFILE);
      return novas_error(5, errno, fn, "reading 'ncon' from '%s': %s", ephem_name, strerror(errno));
    }
    if(fread(&JPLAU, sizeof JPLAU, 1, EPHFILE) != 1) {
      fclose(EPHFILE);
      return novas_error(6, errno, fn, "reading 'JPLAU' from '%s': %s", ephem_name, strerror(errno));
    }
    if(fread(&EM_RATIO, sizeof EM_RATIO, 1, EPHFILE) != 1) {
      fclose(EPHFILE);
      return novas_error(7, errno, fn, "reading 'EM_RATIO' from '%s': %s", ephem_name, strerror(errno));
    }
    for(i = 0; i < 12; i++)
      for(j = 0; j < 3; j++)
        if(fread(&IPT[j][i], sizeof(int), 1, EPHFILE) != 1) {
          fclose(EPHFILE);
          return novas_error(8, errno, fn, "reading 'IPT[%d][%d]' from '%s': %s", j, i, ephem_name, strerror(errno));
        }
    if(fread(&denum, sizeof denum, 1, EPHFILE) != 1) {
      fclose(EPHFILE);
      return novas_error(9, errno, fn, "reading 'denum' from '%s': %s", ephem_name, strerror(errno));
    }
    if(fread(LPT, sizeof LPT, 1, EPHFILE) != 1) {
      fclose(EPHFILE);
      return novas_error(10, errno, fn, "reading 'LPT' from '%s': %s", ephem_name, strerror(errno));
    }

    // Set the value of the record length according to what JPL ephemeris is being opened.
    switch(denum) {
      case 200:
        RECORD_LENGTH = 6608;
        break;
      case 403:
      case 405:
      case 421:
        RECORD_LENGTH = 8144;
        break;
      case 404:
      case 406:
        RECORD_LENGTH = 5824;
        break;

      default:            // An unknown DE file was opened. Close the file and return an error code.
        if(jd_begin)
          *jd_begin = 0.0;
        if(jd_end)
          *jd_end = 0.0;
        if(de_number)
          *de_number = 0;
        fclose(EPHFILE);
        return novas_error(11, errno, fn, "Unknown record size for DE number: %d in '%s'", denum, ephem_name);
        break;
    }

    BUFFER = (double*) calloc(RECORD_LENGTH / 8, sizeof(double));

    if(de_number)
      *de_number = (short) denum;
    if(jd_begin)
      *jd_begin = SS[0];
    if(jd_end)
      *jd_end = SS[1];
  }

  return 0;
}

/**
 * Closes a JPL planetary ephemeris file and frees the memory.
 *
 * REFERENCES:
 * <ol>
 * <li>Standish, E.M. and Newhall, X X (1988). "The JPL Export
 *     Planetary Ephemeris"; JPL document dated 17 June 1988.</li>
 * </ol>
 *
 * NOTES:
 * <ol>
 * <li>Includes fix for the known resource leak issue in NOVAS C 3.1.
 * </ol>
 *
 * @return  0 if the file successfully closed or was closed already, or else EOF.
 *
 * @sa ephem_open()
 *
 */
short ephem_close(void) {
  if(EPHFILE) {
    int error = fclose(EPHFILE);
    EPHFILE = NULL;
    free(BUFFER);
    return novas_error(error, errno, "ephem_close", strerror(errno));
  }
  return 0;
}


/**
 * Retries planet position and velocity data from the JPL planetary ephemeris
 *
 * (If nutations are desired, set 'target' = 13; 'center' will be ignored on that call.)
 *
 * REFERENCES:
 * <ol>
 * <li>Standish, E.M. and Newhall, X X (1988). "The JPL Export
 *     Planetary Ephemeris"; JPL document dated 17 June 1988.</li>
 * </ol>
 *
 * @param tjd       [day] Two-element array containing the Julian date, which may be split any way
 *                  (although the first element is usually the "integer" part, and the second
 *                  element is the "fractional" part).  Julian date is in the TDB or "T_eph" time
 *                  scale.
 * @param target    The integer code (see above) for the planet for which coordinates are
 *                  requested, e.g. DE_JUPITER.
 * @param origin    The integer code of the planet or position relative to which coordinates are
 *                  measured.
 * @param[out] position   [AU] Position vector array of target relative to center.
 * @param[out] velocity   [AU/day] Velocity vector array of target relative to center.
 * @return          0 if successful, or -1 if one of the pointer arguments is NULL, or else the
 *                  error returned from state().
 *
 * @sa ephem_open()
 */
short planet_ephemeris(const double tjd[2], enum de_planet target, enum de_planet origin, double *position,
        double *velocity) {
  static const char *fn = "planet_ephemeris";

  int i;
  int do_earth = 0, do_moon = 0;

  double jed[2];
  double pos_moon[3] = {0}, vel_moon[3] = {0}, pos_earth[3] = {0}, vel_earth[3] = {0};
  double target_pos[3] = {0}, target_vel[3] = {0}, center_pos[3] = {0}, center_vel[3] = {0};

  if(!tjd || !position || !velocity)
    return novas_error(-1, EINVAL, "planet_ephemeris", "NULL parameter: tjd=%p, position=%p, velocity=%p", tjd, position, velocity);

  // Initialize 'jed' for 'state' and set up component count.
  jed[0] = tjd[0];
  jed[1] = tjd[1];

  // Check for target point = center point.
  if(target == origin) {
    for(i = 0; i < 3; i++) {
      position[i] = 0.0;
      velocity[i] = 0.0;
    }
    return 0;
  }

  // Check for instances of target or center being Earth or Moon,
  // and for target or center being the Earth-Moon barycenter.
  if((target == DE_EARTH) || (origin == DE_EARTH))
    do_moon = 1;
  if((target == DE_MOON) || (origin == DE_MOON))
    do_earth = 1;
  if((target == DE_EMB) || (origin == DE_EMB))
    do_earth = 1;

  if(do_earth)
    prop_error(fn, state(jed, DE_EARTH, pos_earth, vel_earth), 0);

  if(do_moon)
    prop_error(fn, state(jed, DE_MOON, pos_moon, vel_moon), 0);

  // Make call to State for target object.
  if(target == DE_SSB) {
    for(i = 0; i < 3; i++) {
      target_pos[i] = 0.0;
      target_vel[i] = 0.0;
    }
  }
  else if(target == DE_EMB) {
    for(i = 0; i < 3; i++) {
      target_pos[i] = pos_earth[i];
      target_vel[i] = vel_earth[i];
    }
  }
  else
    prop_error(fn, state(jed, target, target_pos, target_vel), 0);

  // Make call to State for center object.

  // If the requested center is the Solar System barycenter,
  // then don't bother with a second call to State.
  if(origin == DE_SSB) {
    for(i = 0; i < 3; i++) {
      center_pos[i] = 0.0;
      center_vel[i] = 0.0;
    }
  }

  // Center is Earth-Moon barycenter, which was already computed above.
  else if(origin == DE_EMB) {
    for(i = 0; i < 3; i++) {
      center_pos[i] = pos_earth[i];
      center_vel[i] = vel_earth[i];
    }
  }
  else
    prop_error(fn, state(jed, origin, center_pos, center_vel), 0);

  // Check for cases of Earth as target and Moon as center or vice versa.
  if((target == DE_EARTH) && (origin == DE_MOON)) {
    for(i = 0; i < 3; i++) {
      position[i] = -center_pos[i];
      velocity[i] = -center_vel[i];
    }
    return 0;
  }
  else if((target == DE_MOON) && (origin == DE_EARTH)) {
    for(i = 0; i < 3; i++) {
      position[i] = target_pos[i];
      velocity[i] = target_vel[i];
    }
    return 0;
  }

  // Check for Earth as target, or as center.
  else if(target == DE_EARTH) {
    for(i = 0; i < 3; i++) {
      target_pos[i] = target_pos[i] - (pos_moon[i] / EM1);
      target_vel[i] = target_vel[i] - (vel_moon[i] / EM1);
    }
  }
  else if(origin == DE_EARTH) {
    for(i = 0; i < 3; i++) {
      center_pos[i] = center_pos[i] - (pos_moon[i] / EM1);
      center_vel[i] = center_vel[i] - (vel_moon[i] / EM1);
    }
  }

  // Check for Moon as target, or as center.
  else if(target == DE_MOON) {
    for(i = 0; i < 3; i++) {
      target_pos[i] = (pos_earth[i] - (target_pos[i] / EM1)) + target_pos[i];
      target_vel[i] = (vel_earth[i] - (target_vel[i] / EM1)) + target_vel[i];
    }
  }
  else if(origin == DE_MOON) {
    for(i = 0; i < 3; i++) {
      center_pos[i] = (pos_earth[i] - (center_pos[i] / EM1)) + center_pos[i];
      center_vel[i] = (vel_earth[i] - (center_vel[i] / EM1)) + center_vel[i];
    }
  }

  // Compute position and velocity vectors.
  for(i = 0; i < 3; i++) {
    position[i] = target_pos[i] - center_pos[i];
    velocity[i] = target_vel[i] - center_vel[i];
  }

  return 0;
}

/**
 * Reads and interpolates the JPL planetary ephemeris file.
 *
 * For ease in programming, the user may put the entire epoch in jed[0] and set jed[1] = 0.
 * For maximum interpolation accuracy, set jed[0] = the most recent midnight at or before
 * interpolation epoch, and set jed[1] = fractional part of a day elapsed between jed[0] and
 * epoch. As an alternative, it may prove convenient to set jed[0] = some fixed epoch, such
 * as start of the integration and jed[1] = elapsed interval between then and epoch.
 *
 * REFERENCES:
 * <ol>
 * <li>Standish, E.M. and Newhall, X X (1988). "The JPL Export
 *     Planetary Ephemeris"; JPL document dated 17 June 1988.</li>
 * </ol>
 *
 * @param jed         [day] 2-element Julian date (TDB) at which interpolation is wanted. Any
 *                    combination of jed[0]+jed[1] which falls within the time span on the file is
 *                    a permissible epoch.  See Note 1 below.
 * @param target      The integer code (see above) for the planet for which coordinates are
 *                    requested, e.g. DE_JUPITER.
 * @param[out] target_pos   [AU] The barycentric position vector array of the requested object.
 * @param[out] target_vel   [AU/day] The barycentric velocity vector array of the requested
 *                          object.
 *
 * @return            0 if successful, -1 if any of the pointer arguments is NULL, or else 1 if
 *                    there was an error reading the ephemeris file, or 2 if the epoch is out of
 *                    range.
 */
short state(const double *jed, enum de_planet target, double *target_pos, double *target_vel) {
  static const char *fn = "state";
  long nr;
  double t[2], aufac = 1.0, jd[4], s;
  int i;

  if(!jed || !target_pos || !target_vel)
    return novas_error(-1, EINVAL, fn, "NULL parameter: jed=%p, target_pos=%p, target_vel=%p", jed, target_pos, target_vel);

  // Set units based on value of the 'KM' flag.
  if(KM)
    t[1] = SS[2] * 86400.0;
  else {
    t[1] = SS[2];
    aufac = 1.0 / JPLAU;
  }

  // Check epoch.
  s = jed[0] - 0.5;
  split(s, &jd[0]);
  split(jed[1], &jd[2]);
  jd[0] += jd[2] + 0.5;
  jd[1] += jd[3];
  split(jd[1], &jd[2]);
  jd[0] += jd[2];

  // Return error code if date is out of range.
  if((jd[0] < SS[0]) || ((jd[0] + jd[3]) > SS[1]))
    return novas_error(2, EDOM, fn, "date (JD=%.1f) is out of range", jed[0] + jed[1]);

  // Calculate record number and relative time interval.
  nr = (long) ((jd[0] - SS[0]) / SS[2]) + 3;
  if(jd[0] == SS[1])
    nr -= 2;
  t[0] = ((jd[0] - ((double) (nr - 3) * SS[2] + SS[0])) + jd[3]) / SS[2];

  // Read correct record if it is not already in memory.
  if(nr != NRL) {
    long rec;
    NRL = nr;
    rec = (nr - 1) * RECORD_LENGTH;
    fseek(EPHFILE, rec, SEEK_SET);
    if(!fread(BUFFER, RECORD_LENGTH, 1, EPHFILE)) {
      ephem_close();
      return novas_error(1, errno, fn, "reading record %ld: %s", nr, strerror(errno));
    }
  }

  // Check and interpolate for requested body.
  interpolate(&BUFFER[IPT[0][target] - 1], t, IPT[1][target], IPT[2][target], target_pos, target_vel);

  for(i = 0; i < 3; i++) {
    target_pos[i] *= aufac;
    target_vel[i] *= aufac;
  }

  return 0;
}

/**
 * Differentiates and interpolates a set of Chebyshev coefficients to give position and velocity.
 *
 * REFERENCES:
 * <ol>
 * <li>Standish, E.M. and Newhall, X X (1988). "The JPL Export
 *     Planetary Ephemeris"; JPL document dated 17 June 1988.</li>
 * </ol>
 *
 * @param buf           Array of Chebyshev coefficients of position.
 * @param t             t[0] is fractional time interval covered by coefficients at which
 *                      interpolation is desired (0 <= t[0] <= 1). t[1] is length of whole
 *                      interval in input time units.
 * @param ncf           Number of coefficients per component.
 * @param na            Number of sets of coefficients in full array (i.e., number of
 *                      sub-intervals in full interval).
 * @param[out] position   Position array of requested object.
 * @param[out] velocity   Velocity array of requested object.
 *
 * @return              0 if successful, or -1 if one of the input arrays or output pointer
 *                      arguments is NULL.
 */
int interpolate(const double *buf, const double *t, long ncf, long na, double *position, double *velocity) {
  long i, j, k, l;

  double dna, dt1, temp, tc, vfac;

  if(!buf || !t || !position || !velocity)
    return novas_error(-1, EINVAL, "interpolate", "NULL parameter: buf=%p, t=%p, position=%p, velocity=%p", buf, t, position, velocity);

  // Get correct sub-interval number for this set of coefficients and
  // then get normalized Chebyshev time within that subinterval.
  dna = (double) na;
  dt1 = (double) ((long) t[0]);
  temp = dna * t[0];
  l = (long) (temp - dt1);

  // 'tc' is the normalized Chebyshev time (-1 <= tc <= 1).
  tc = 2.0 * (remainder(temp, 1.0) + dt1) - 1.0;

  // Check to see whether Chebyshev time has changed, and compute new
  // polynomial values if it has.  (The element PC[1] is the value of
  // t1[tc] and hence contains the value of 'tc' on the previous call.)
  if(tc != PC[1]) {
    NP = 2;
    NV = 3;
    PC[1] = tc;
    TWOT = tc + tc;
  }

  // Be sure that at least 'ncf' polynomials have been evaluated and
  // are stored in the array 'PC'.
  if(NP < ncf) {
    for(i = NP; i < ncf; i++)
      PC[i] = TWOT * PC[i - 1] - PC[i - 2];
    NP = ncf;
  }

  // Interpolate to get position for each component.
  for(i = 0; i < 3; i++) {
    position[i] = 0.0;
    for(j = ncf - 1; j >= 0; j--) {
      k = j + (i * ncf) + (l * (3 * ncf));
      position[i] += PC[j] * buf[k];
    }
  }

  // If velocity interpolation is desired, be sure enough derivative
  // polynomials have been generated and stored.
  vfac = (2.0 * dna) / t[1];
  VC[2] = 2.0 * TWOT;
  if(NV < ncf) {
    for(i = NV; i < ncf; i++)
      VC[i] = TWOT * VC[i - 1] + PC[i - 1] + PC[i - 1] - VC[i - 2];
    NV = ncf;
  }

  // Interpolate to get velocity for each component.
  for(i = 0; i < 3; i++) {
    velocity[i] = 0.0;
    for(j = ncf - 1; j > 0; j--) {
      k = j + (i * ncf) + (l * (3 * ncf));
      velocity[i] += VC[j] * buf[k];
    }
    velocity[i] *= vfac;
  }

  return 0;
}

/**
 * Breaks up a double number into a double integer part and a fractional part.
 *
 * @param tt        Input number.
 * @param[out] fr   2-element output array; fr[0] contains integer part, fr[1] contains
 *                  fractional part. For negative input numbers, fr[0] contains the next
 *                  more negative integer; fr[1] contains a positive fraction.
 *
 * @return          0 if successful, or -1 if the output pointer argument is NULL.
 */
int split(double tt, double *fr) {
  if(!fr)
    return novas_error(-1, EINVAL, "split", "NULL output pointer");

  // Get integer and fractional parts.
  fr[0] = floor(tt);
  fr[1] = tt - fr[0];
  return 0;
}

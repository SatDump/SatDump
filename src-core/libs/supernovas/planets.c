/**
 * @file
 *
 * @date Created  on Apr 27, 2025
 * @author Attila Kovacs
 *
 *  Calculate approximate positions and velocities for the major planets, Sun, Moon, Earth-Moon
 *  Barycenter (EMB), and the Solar-system Barycenter (SSB), mainly by using Keplerian orbital
 *  elements.
 */

#include <math.h>
#include <string.h>
#include <errno.h>

/// \cond PRIVATE
#define __NOVAS_INTERNAL_API__    ///< Use definitions meant for internal use by SuperNOVAS only
#include "novas.h"
/// \endcond

/**
 * Returns the planetary longitude, for Mercury through Neptune, w.r.t. mean dynamical
 * ecliptic and equinox of J2000, with high order terms omitted (Simon et al. 1994,
 * 5.8.1-5.8.8).
 *
 * REFERENCES:
 * <ol>
 * <li>IERS Conventions Chapter 5, Eq. 5.44.</li>
 * </ol>
 *
 * @param t       [cy] Julian centuries since J2000
 * @param planet  Novas planet id, e.g. NOVAS_MARS.
 * @return        [rad] The approximate longitude of the planet in radians [-&pi;:&pi;],
 *                or NAN if the `planet` id is out of range.
 *
 * @sa accum_prec(), nutation_angles(), NOVAS_JD_J2000
 *
 * @since 1.0
 * @author Attila Kovacs
 */
double planet_lon(double t, enum novas_planet planet) {
  static const double c[9][2] = {
          { 0.0, 0.0 }, //
          { 4.402608842461, 2608.790314157421 },  // Mercury
          { 3.176146696956, 1021.328554621099 },  // Venus
          { 1.753470459496,  628.307584999142 },  // Earth
          { 6.203476112911,  334.061242669982 },  // Mars
          { 0.599547105074,   52.969096264064 },  // Jupiter
          { 0.874016284019,   21.329910496032 },  // Saturn
          { 5.481293871537,    7.478159856729 },  // Uranus
          { 5.311886286677,    3.813303563778 }   // Neptune
  };

  if(planet < NOVAS_MERCURY || planet > NOVAS_NEPTUNE) {
    novas_set_errno(EINVAL, "planet_lon", "invalid planet number: %d", planet);
    return NAN;
  }

  return remainder(c[planet][0] + c[planet][1] * t, TWOPI);
}

/**
 * Get approximate current heliocentric orbital elements for the major planets. These orbital
 * elements are not suitable for precise position velocity calculations, but they may be
 * useful to obtain approximate positions for the major planets, e.g. to estimate rise or set
 * times, or apparent elevation angles from an observing site.
 *
 * These orbitals can provide planet positions to arcmin-level precision for the rocky inner
 * planets, and to a fraction of a degree precision for the gas and ice giants and Pluto. The
 * accuracies for Uranus, Neptune, and Pluto are significantly improved (to the arcmin level) if
 * used in the time range of 1800 AD to 2050 AD. For a more detailed summary of the typical
 * accuracies, see either of the references below.
 *
 * NOTES:
 * <ol>
 *  <li>The Earth-Moon system is treated as a single orbital of the Earth-Moon Barycenter (EMB).
 *      That is, the EMB orbital is returned for both Earth and the Moon also.</li>
 *  <li>For Pluto, the Pluto system barycenter orbit is returned.</li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 *  <li>E.M. Standish and J.G. Williams 1992.</li>
 *  <li>https://ssd.jpl.nasa.gov/planets/approx_pos.html</li>
 * </ol>
 *
 * @param id          NOVAS major planet ID. All major planets, except Earth, are supported. The
 *                    Earth-Moon Barycenter (EMB), and Pluto system Barycenter are supported also.
 *                    (For Pluto, the Pluto System Barycenter values are returned.)
 * @param jd_tdb      [day] Barycentric Dynamical Time (TDB) based Julian Date.
 * @param[out] orbit  Orbital elements data structure to populate.
 * @return            0 if successful, or else -1 (`errno` set to `EINVAL`).
 *
 * @since 1.4
 * @author Attila Kovacs
 *
 * @sa novas_make_moon_orbit(), novas_approx_sky_pos(), novas_approx_heliocentric(),
 *     make_orbital_object()
 */
int novas_make_planet_orbit(enum novas_planet id, double jd_tdb, novas_orbital *restrict orbit) {
  static const char *fn = "novas_make_planet_orbit";

  typedef struct {
    double a;         // [AU] Semi-major axis
    double e;         // eccentricity
    double I;         // [deg] inclination
    double L;         // [deg] longitude at reference epoch
    double omega_bar; // [deg] logitude of perihelion
    double Omega;     // [deg] longitude of rising node
  } planet_elements;

  typedef struct {
    double b;
    double c;
    double s;
    double f;
  } planet_terms;

  /**
   * Keplerian orbital elements at J2000 from Table 8.10.2 of E.M. Standish and J.G. Williams 1992,
   * valid for 1800 AD to 2050 AD.
   */
  static const planet_elements ref[] = {
          //     a                e                I                 L             long.peri.       long.node.
          //    [au]            [rad]            [deg]             [deg]             [deg]            [deg]
          {  0.38709927,      0.20563593,      7.00497902,      252.25032350,     77.45779628,     48.33076593 }, // Mercury
          {  0.72333566,      0.00677672,      3.39467605,      181.97909950,    131.60246718,     76.67984255 }, // Venus
          {  1.00000261,      0.01671123,     -0.00001531,      100.46457166,    102.93768193,      0.0        }, // EMB
          {  1.52371034,      0.09339410,      1.84969142,       -4.55343205,    -23.94362959,     49.55953891 }, // Mars
          {  5.20288700,      0.04838624,      1.30439695,       34.39644051,     14.72847983,    100.47390909 }, // Jupiter
          {  9.53667594,      0.05386179,      2.48599187,       49.95424423,     92.59887831,    113.66242448 }, // Saturn
          { 19.18916464,      0.04725744,      0.77263783,      313.23810451,    170.95427630,     74.01692503 }, // Uranus
          { 30.06992276,      0.00859048,      1.77004347,      -55.12002969,     44.96476227,    131.78422574 }, // Neptune
          { 39.48211675,      0.24882730,     17.14001206,      238.92903833,    224.06891629,    110.30393684 }  // Pluto
  };

  /**
   * Temporal evolution of the Keplerian orbital elements from Table 8.10.2 of E.M. Standish and
   * J.G. Williams 1992, valid for 1800 AD to 2050 AD.
   */
  static const planet_elements dot[] = {

          //     a                e                I                 L             long.peri.       long.node.
          //   [au/cy]         [rad/cy]         [deg/cy]         [deg/cy]           [deg/cy]         [deg/cy]
          {  0.00000037,      0.00001906,     -0.00594749,   149472.67411175,      0.16047689,     -0.12534081 }, // Mercury
          {  0.00000390,     -0.00004107,     -0.00078890,    58517.81538729,      0.00268329,     -0.27769418 }, // Venus
          {  0.00000562,     -0.00004392,     -0.01294668,    35999.37244981,      0.32327364,      0.0        }, // EMB
          {  0.00001847,      0.00007882,     -0.00813131,    19140.30268499,      0.44441088,     -0.29257343 }, // Mars
          { -0.00011607,     -0.00013253,     -0.00183714,     3034.74612775,      0.21252668,      0.20469106 }, // Jupiter
          { -0.00125060,     -0.00050991,      0.00193609,     1222.49362201,     -0.41897216,     -0.28867794 }, // Saturn
          { -0.00196176,     -0.00004397,     -0.00242939,      428.48202785,      0.40805281,      0.04240589 }, // Uranus
          {  0.00026291,      0.00005105,      0.00035372,      218.45945325,     -0.32241464,     -0.00508664 }, // Neptune
          { -0.00031596,      0.00005170,      0.00004818,      145.20780515,     -0.04062942,     -0.01183482 }  // Pluto
  };

  /**
   * Keplerian orbital elements at J2000 from Table 8.10.3 of E.M. Standish and J.G. Williams 1992,
   * valid for 3000 BC to 3000 AD.
   */
  static const planet_elements refl[] = {
          //     a                e                I                 L             long.peri.       long.node.
          //    [au]            [rad]            [deg]             [deg]             [deg]            [deg]
          {  0.38709843,      0.20563661,      7.00559432,      252.25166724,     77.45771895,     48.33961819 }, // Mercury
          {  0.72332102,      0.00676399,      3.39777545,      181.97970850,    131.76755713,     76.67261496 }, // Venus
          {  1.00000018,      0.01673163,     -0.00054346,      100.46691572,    102.93005885,     -5.11260389 }, // EMB
          {  1.52371243,      0.09336511,      1.85181869,       -4.56813164,    -23.91744784,     49.71320984 }, // Mars
          {  5.20248019,      0.04853590,      1.29861416,       34.33479152,     14.27495244,    100.29282654 }, // Jupiter
          {  9.54149883,      0.05550825,      2.49424102,       50.07571329,     92.86136063,    113.63998702 }, // Saturn
          { 19.18797948,      0.04685740,      0.77298127,      314.20276625,    172.43404441,     73.96250215 }, // Uranus
          { 30.06952752,      0.00895439,      1.77005520,      304.22289287,     46.68158724,    131.78635853 }, // Neptune
          { 39.48686035,      0.24885238,      17.14104260,     238.96535011,    224.09702598,    110.30167986 }  // Pluto
  };

  /**
   * Temporal evolution of the Keplerian orbital elements from Table 8.10.3 of E.M. Standish and
   * J.G. Williams 1992, vaid for 3000 BC to 3000 AD.
   */
  static const planet_elements dotl[] = {
          //     a                e                I                 L             long.peri.       long.node.
          //   [au/cy]         [rad/cy]         [deg/cy]         [deg/cy]           [deg/cy]         [deg/cy]
          {  0.00000000,      0.00002123,     -0.00590158,   149472.67486623,      0.15940013,     -0.12214182 }, // Mercury
          { -0.00000026,     -0.00005107,      0.00043494,    58517.81560260,      0.05679648,     -0.27274174 }, // Venus
          { -0.00000003,     -0.00003661,     -0.01337178,    35999.37306329,      0.31795260,     -0.24123856 }, // EMB
          {  0.00000097,      0.00009149,     -0.00724757,    19140.29934243,      0.45223625,     -0.26852431 }, // Mars
          { -0.00002864,      0.00018026,     -0.00322699,     3034.90371757,      0.18199196,      0.13024619 }, // Jupiter
          { -0.00003065,     -0.00032044,      0.00451969,     1222.11494724,      0.54179478,     -0.25015002 }, // Saturn
          { -0.00020455,     -0.00001550,     -0.00180155,      428.49512595,      0.09266985,      0.05739699 }, // Uranus
          {  0.00006447,      0.00000818,      0.00022400,      218.46515314,      0.01009938,     -0.00606302 }, // Neptune
          {  0.00449751,      0.00006016,      0.00000501,      145.18042903,     -0.00968827,     -0.00809981 }  // Pluto
  };

  /**
   * Additional terms for computing M for the outer planets (Jupiter and beyond) from Table 8.10.4
   * of E.M. Standish and J.G. Williams 1992.
   */
  static const planet_terms plts[5] = {
          { -0.00012452,    0.06064060,   -0.35635438,   38.35125000 },     // Jupiter
          {  0.00025899,   -0.13434469,    0.87320147,   38.35125000 },     // Saturn
          {  0.00058331,   -0.97731848,    0.17689245,    7.67025000 },     // Uranus
          { -0.00041348,    0.68346318,   -0.10162547,    7.67025000 },     // Neptune
          { -0.01262724,    0.0       ,    0.0       ,    0.0        }      // Pluto
  };

  novas_orbital init = NOVAS_ORBIT_INIT;
  const planet_elements *p0, *p1;
  double t;

  if(!orbit)
    return novas_error(-1, EINVAL, fn, "output orbital is NULL");

  if(id == NOVAS_EARTH)
    return novas_error(-1, EINVAL, fn, "No Earth orbital, but there is one for the E-M Barycenter...\n");

  if(id == NOVAS_EMB)
    id = NOVAS_EARTH; // For EMB the data is under the index fo Earth
  else if(id == NOVAS_PLUTO_BARYCENTER)
    id = NOVAS_PLUTO; // Treat Pluto Barycenter the same as Pluto.
  else if(id < NOVAS_MERCURY || id > NOVAS_PLUTO)
    return novas_error(-1, EINVAL, fn, "unsupported planet id: %d", id);

  t = (jd_tdb - NOVAS_JD_J2000) / JULIAN_CENTURY_DAYS;

  if(t >= -2.0 && t <= 0.5) {
    p0 = &ref[id - 1];
    p1 = &dot[id - 1];
  }
  else if(t >= -30.0 && t <= 30.0) {
    p0 = &refl[id - 1];
    p1 = &dotl[id - 1];
  }
  else
    return novas_error(-1, EINVAL, fn, "time outside of supported range");

  // Default ecliptic orbital...
  memcpy(orbit, &init, sizeof(novas_orbital));

  // Values expressed for instant
  orbit->jd_tdb = jd_tdb;

  orbit->a = (p0->a + p1->a * t);
  orbit->e = (p0->e + p1->e * t);
  orbit->i = (p0->I + p1->I * t);
  orbit->omega = remainder(p0->omega_bar + p1->omega_bar * t, DEG360);      // -> omega_bar
  orbit->Omega = remainder(p0->Omega + p1->Omega * t, DEG360);
  orbit->M0 = remainder(p0->L + p1->L * t - orbit->omega, DEG360);

  orbit->n = p1->L - p1->omega_bar; // M0 = L - omega_bar
  orbit->omega -= orbit->Omega; // omega = omega_bar - Omega

  if(id >= NOVAS_JUPITER && p0 == &refl[id - 1]) {
    // Additional terms for the long-term orbitals only...
    const planet_terms *terms = &plts[id - NOVAS_JUPITER];
    double ft = terms->f * DEGREE * t;
    orbit->M0 += terms->b * t * t + terms->c * cos(ft) + terms->s * sin(ft);
    orbit->n += 2.0 * terms->b * t + terms->f * (terms->s * cos(ft) - terms->c * sin(ft));
  }

  orbit->n /= JULIAN_CENTURY_DAYS;
  orbit->apsis_period = JULIAN_CENTURY_DAYS * TWOPI / p1->omega_bar;
  orbit->node_period = JULIAN_CENTURY_DAYS * TWOPI / p1->Omega;

  return 0;
}

/**
 * Gets the current orbital elements for the Moon relative to the geocenter for the specified
 * epoch of observation.
 *
 * REFERENCES:
 * <ol>
 *  <li>Chapront, J. et al., 2002, A&A 387, 700–709</li>
 *  <li>Chapront-Touze, M, and Chapront, J. 1983, Astronomy and Astrophysics (ISSN 0004-6361),
 *      vol. 124, no. 1, July 1983, p. 50-62.</li>
 * </ol>
 *
 * @param jd_tdb      [day] Barycentric Dynamical Time (TDB) based Julian Date.
 * @param[out] orbit  Orbital elements data structure to populate.
 * @return            0 if successful, or else -1 (`errno` set to `EINVAL`).
 *
 * @since 1.4
 * @author Attila Kovacs
 *
 * @sa novas_make_planet_orbit(), make_orbital_object()
 */
int novas_make_moon_orbit(double jd_tdb, novas_orbital *restrict orbit) {
  novas_orbital def = NOVAS_ORBIT_INIT;
  double t, dot;

  if(!orbit)
    return novas_error(-1, EINVAL, "novas_make_moon_orbit", "output orbital is NULL");

  // Default ecliptic orbital...
  memcpy(orbit, &def, sizeof(novas_orbital));

  orbit->system.center = NOVAS_EARTH;

  // Values expressed for instant
  orbit->jd_tdb = jd_tdb;

  t = (jd_tdb - NOVAS_JD_J2000) / JULIAN_CENTURY_DAYS;

  // From Chapront-Touze, M, and Chapront, J. 1983, A&A, 124, 1, p. 50-62.
  // ELP2000 model.
  orbit->i = 5.128121856; // 2 Gamma
  orbit->e = 0.01670924 + t * (-0.42037954e-4 - t * 0.122196e-6);

  // From Chapront, J. et al., 2002, A&A 387, 700–709
  orbit->M0 = 218.316632833333 + t * (481266.484259111 + t * (-0.0019083 + t * (1.8344e-06 - t * 8.8028e-09)));    // W1
  orbit->omega = 83.3532366111111 + t * (4067.61675844444 + t * (-0.010629 + t * (-1.25131e-5 + t * 5.91694e-8))); // W2
  orbit->Omega = 125.044535138889 + t * (-1935.53330141667 + t * (0.00176647 + t * (2.1181e-6 - t * 9.9611e-9)));  // W3

  // treat omega
  orbit->M0 -= orbit->omega;
  orbit->omega -= orbit->Omega;

  // differentiate M0 above to get mean motion
  orbit->n = 481266.484259111 + t * (-0.0038166 + t * (5.5032e-06 - t * 7.92252e-08));

  // From Chapront-Touze, M, and Chapront, J. 1983, A&A, 124, 1, p. 50-62.
  // (n^2 a^3 = constant).
  orbit->a = 3.84747980645e8 / NOVAS_AU * pow(481266.484259111 / orbit->n, 2.0 / 3.0);

  orbit->n /= JULIAN_CENTURY_DAYS;

  // differentiate omega above to get apsis motion
  dot = 4067.61675844444 + t * (-0.021258 + t * (-3.75393e-05 + t * 2.366776e-07));
  orbit->apsis_period = JULIAN_CENTURY_DAYS * TWOPI / dot;

  // differentiate Omega above to get node motion
  dot = -1935.53330141667 + t * (0.00353294 + t * (6.3543e-06 - t * 3.98444e-08));
  orbit->node_period = JULIAN_CENTURY_DAYS * TWOPI / dot;

  return 0;
}

/**
 * Returns the approximate geometric heliocentric orbital positions and velocities for the major
 * planets, Sun, or Earth-Moon Barycenter (EMB). The returned positions and velocities are not
 * suitable for precise calculations. However, they may be used to provide rough guidance about
 * the approximate locations of the planets, e.g. for determining approximate rise or set times or
 * the approximate azimuth / elevation angles from an observing site.
 *
 * The orbitals can provide planet positions to arcmin-level precision for the rocky inner
 * planets, and to a fraction of a degree precision for the gas and ice giants and Pluto. The
 * accuracies for Uranus, Neptune, and Pluto are significantly improved (to the arcmin level) if
 * used in the time range of 1800 AD to 2050 AD. For a more detailed summary of the typical
 * accuracies, see either of the top two references below.
 *
 * For accurate positions, you should use planetary ephemerides (such as the JPL ephemerides via
 * the CALCEPH or CSPICE plugins) and `novas_geom_posvel()` instead.
 *
 * While this function is generally similar to creating an orbital object with an orbit
 * initialized with `novas_make_planet_orbit()` or `novas_make_moon_orbit()`, and then calling
 * `novas_geom_posvel()`, there are a few important differences:
 *
 * <ol>
 *  <li>This function returns geometric positions referenced to the Sun (i.e., heliocentric),
 *  whereas `novas_geom_posvel()` references the calculated positions to the Solar-system
 *  Barycenter (SSB).</li>
 *  <li>This function calculates Earth and Moon positions about the Keplerian orbital position
 *  of the Earth-Moon Barycenter (EMB). In constrast, `novas_make_planet_orbit()` does not provide
 *  orbitals for the Earth directly, and `make_moot_orbit()` references the Moon's orbital to
 *  the Earth position returned by the currently configured planet calculator function (see
 *  `set_planet_provider()`).</li>
 * </ol>
 *
 *
 * NOTES:
 * <ol>
 *  <li>The Sun's position w.r.t. the Solar-system Barycenter is calculated using
 *  `earth_sun_calc()`. All other orbitals are also referenced to the Sun's position calculated
 *  that way.</li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 *  <li>E.M. Standish and J.G. Williams 1992.</li>
 *  <li>https://ssd.jpl.nasa.gov/planets/approx_pos.html</li>
 *  <li>Chapront, J. et al., 2002, A&A 387, 700–709</li>
 *  <li>Chapront-Touze, M, and Chapront, J. 1983, Astronomy and Astrophysics (ISSN 0004-6361),
 *      vol. 124, no. 1, July 1983, p. 50-62.</li>
 * </ol>
 *
 * @param id        NOVAS major planet ID. All major planets, plus the Sun, Moon, Earth-Moon
 *                  Barycenter (EMB), and Pluto system Barycenter are supported. (For Pluto, the
 *                  Pluto System Barycenter value are returned.)
 * @param jd_tdb    [day] Barycentric Dynamical Time (TDB) based Julian Date. Dates between
 *                  3000 BC and 3000 AD are supported. For dates between 1800 AD and 2050
 *                  AD the returned positions are somewhat more accurate.
 * @param[out] pos  [AU] Output Heliocentric ICRS position vector, or NULL if not required.
 * @param[out] vel  [AU/day] Output Heliocentric ICRS velocity vector, or NULL if not required.
 * @return          0 if successful, or if the JD date is outside of the range with valid
 *                  parameters, or else -1 if the planet ID is not supported or if both
 *                  output vectors are NULL. In case of errors errno will be set to indicate
 *                  the type of error.
 *
 * @since 1.4
 * @author Attila Kovacs
 *
 * @sa novas_approx_sky_pos(), earth_sun_calc(), novas_geom_posvel()
 * @sa novas_use_calceph(), novas_use_cspice()
 */
int novas_approx_heliocentric(enum novas_planet id, double jd_tdb, double *restrict pos, double *restrict vel) {
  static const char *fn = "novas_approx_heliocentric";

  novas_orbital orbit = NOVAS_ORBIT_INIT;

  if(pos == vel)
    return novas_error(-1, EINVAL, fn, "output pos = vel");

  if(pos)
    memset(pos, 0, XYZ_VECTOR_SIZE);
  if(vel)
    memset(vel, 0, XYZ_VECTOR_SIZE);

  switch(id) {
    case NOVAS_SUN:
      break;

    case NOVAS_EARTH:
    case NOVAS_MOON: {
      double pm[3] = {0.0}, vm[3] = {0.0};
      const double r = 0.012150585609632; // Moon to Earth-Moon system mass ratio
      const double f = (id == NOVAS_MOON) ? (1.0 - r) : -r;   // E-M distance fraction from EMB
      int i;

      prop_error(fn, novas_make_planet_orbit(NOVAS_EMB, jd_tdb, &orbit), 0);
      prop_error(fn, novas_orbit_posvel(jd_tdb, &orbit, NOVAS_REDUCED_ACCURACY, pos, vel), 0);

      // Geocentric orbital elements of the Moon.
      prop_error(fn, novas_make_moon_orbit(jd_tdb, &orbit), 0);
      prop_error(fn, novas_orbit_posvel(jd_tdb, &orbit, NOVAS_REDUCED_ACCURACY, pm, vm), 0);

      for(i = 3; --i >= 0; ) {
        if(pos)
          pos[i] += f * pm[i];
        if(vel)
          vel[i] += f * vm[i];
      }
      break;
    }

    default:
      prop_error(fn, novas_make_planet_orbit(id, jd_tdb, &orbit), 0);
      prop_error(fn, novas_orbit_posvel(jd_tdb, &orbit, NOVAS_REDUCED_ACCURACY, pos, vel), 0);
  }

  return 0;
}

/**
 * Calculates an approximate apparent location on sky for a major planet, Sun, Moon, Earth-Moon
 * Barycenter (EMB) -- typically to arcmin level accuracy -- using Keplerian orbital elements. The
 * returned position is antedated for light-travel time (for Solar-System bodies). It also applies
 * an appropriate aberration correction (but not gravitational deflection).
 *
 * The orbitals can provide planet positions to arcmin-level precision for the rocky inner
 * planets, and to a fraction of a degree precision for the gas and ice giants and Pluto. The
 * accuracies for Uranus, Neptune, and Pluto are significantly improved (to the arcmin level) if
 * used in the time range of 1800 AD to 2050 AD. For a more detailed summary of the typical
 * accuracies, see either of the top two references below.
 *
 * For accurate positions, you should use planetary ephemerides (such as the JPL ephemerides via
 * the CALCEPH or CSPICE plugins) and `novas_sky_pos()` instead.
 *
 * While this function is generally similar to creating an orbital object with an orbit
 * initialized with `novas_make_planet_orbit()` or `novas_make_moon_orbit()`, and then calling
 * `novas_sky_pos()`, there are a few important differences to note:
 *
 * <ol>
 *  <li>This function calculates Earth and Moon positions about the Keplerian orbital position
 *  of the Earth-Moon Barycenter (EMB). In constrast, `novas_make_planet_orbit()` does not provide
 *  orbitals for the Earth directly, and `make_moot_orbit()` references the Moon's orbital to
 *  the Earth position returned by the currently configured planet calculator function (see
 *  `set_planet_provider()`).</li>
 *  <li>This function ignores gravitational deflection. It makes little sense to bother about
 *  corrections that are orders of magnitude below the accuracy of the orbital positions
 *  obtained.</li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 *  <li>E.M. Standish and J.G. Williams 1992.</li>
 *  <li>https://ssd.jpl.nasa.gov/planets/approx_pos.html</li>
 *  <li>Chapront, J. et al., 2002, A&A 387, 700–709</li>
 *  <li>Chapront-Touze, M, and Chapront, J. 1983, Astronomy and Astrophysics (ISSN 0004-6361),
 *      vol. 124, no. 1, July 1983, p. 50-62.</li>
 * </ol>
 *
 * @param id            NOVAS major planet ID. All major planets, plus the Sun, Moon, Earth-Moon
 *                      Barycenter (EMB), and Pluto system Barycenter are supported. (For Pluto,
 *                      the Pluto System Barycenter values are returned.)
 * @param frame         The observer frame, defining the location and time of observation.
 * @param sys           The coordinate system in which to return the apparent sky location.
 * @param[out] out      Pointer to the data structure which is populated with the calculated
 *                      approximate apparent location in the designated coordinate system.
 * @return              0 if successful, or else -1 in case of an error (errno will indicate
 *                      the type of error).
 *
 * @since 1.4
 * @author Attila Kovacs
 *
 * @sa novas_sky_pos(), novas_app_to_hor()
 * @sa novas_make_frame()
 */
int novas_approx_sky_pos(enum novas_planet id, const novas_frame *restrict frame, enum novas_reference_system sys, sky_pos *restrict out) {
  static const char *fn = "novas_approx_sky_pos";

  object pl;
  double d_sun, pos[3], vel[3];
  int k;

  if(!frame)
    return novas_error(-1, EINVAL, fn, "frame is NULL");

  if(!out)
    return novas_error(-1, EINVAL, fn, "output sky_pos is NULL");

  if(!novas_frame_is_initialized(frame))
    return novas_error(-1, EINVAL, fn, "frame at %p not initialized", frame);

  prop_error(fn, make_planet(id, &pl), 0);
  prop_error(fn, novas_approx_heliocentric(id, novas_get_time(&frame->time, NOVAS_TDB), pos, vel), 0);

  d_sun = novas_vlen(pos);

  for(k = 3; --k >= 0;) {
    // Heliocentric -> observer pos.
    pos[k] += frame->sun_pos[k] - frame->obs_pos[k];
    // Heliocentric -> barycentric vel.
    vel[k] += frame->sun_vel[k];
  }

  prop_error(fn, novas_geom_to_app(frame, pos, sys, out), 0);

  out->dis = novas_vlen(pos);
  out->rv = rad_vel2(&pl, pos, vel, pos, frame->obs_vel, novas_vdist(frame->obs_pos, frame->earth_pos),
          novas_vdist(frame->obs_pos, frame->sun_pos), d_sun);

  return 0;
}

/**
 * Calculates the Moon's phase at a given time. It uses orbital models for Earth (E.M. Standish
 * and J.G. Williams 1992), and for the Moon (Chapront, J. et al., 2002), and takes into account
 * the slightly eccentric nature of both orbits.
 *
 * NOTES:
 * <ol>
 * <li>The Moon's phase here follows the definition by the Astronomical Almanac, as the excess
 * ecliptic longitude of the Moon over that of the Sun seen from the geocenter.</li>
 * <li>There are other definitions of the phase too, depending on which you might find slightly
 * different answers, but regardless of the details most phase calculations should match to within
 * a few degrees.</li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 *  <li>The Explanatory Supplement to the Astronomical Almanac, University Science Books, 3rd ed.,
 *      p. 507</li>
 *  <li>E.M. Standish and J.G. Williams 1992.</li>
 *  <li>https://ssd.jpl.nasa.gov/planets/approx_pos.html</li>
 *  <li>Chapront, J. et al., 2002, A&A 387, 700–709</li>
 *  <li>Chapront-Touze, M, and Chapront, J. 1983, Astronomy and Astrophysics (ISSN 0004-6361),
 *      vol. 124, no. 1, July 1983, p. 50-62.</li>
 * </ol>
 *
 * @param jd_tdb      [day] Barycentric Dynamical Time (TDB) based Julian Date.
 * @return            [deg] The Moon's phase, or more precisely the ecliptic longitude difference
 *                    between the Sun and the Moon, as seen from the geocenter. 0: New Moon, 90:
 *                    1st quarter, +/- 180 Full Moon, -90: 3rd quarter or NAN if the solution
 *                    failed to converge (errno will be set to ECANCELED), or if the JD date is
 *                    outside the range of the orbital model (errno set to EINVAL).
 *
 * @since 1.4
 * @author Attila Kovacs
 *
 * @sa novas_next_moon_phase(), novas_make_moon_orbit(), novas_solar_illum()
 */
double novas_moon_phase(double jd_tdb) {
  static const char *fn = "novas_moon_phase";

  novas_orbital orbit = NOVAS_ORBIT_INIT;
  double pos[3] = {0.0};
  double he, hm;

  // EMB pos around Sun
  prop_nan(fn, novas_make_planet_orbit(NOVAS_EMB, jd_tdb, &orbit));
  prop_nan(fn, novas_orbit_native_posvel(jd_tdb, &orbit, pos, NULL));
  vector2radec(pos, &he, NULL);

  // Moon pos around Earth
  novas_make_moon_orbit(jd_tdb, &orbit);
  prop_nan(fn, novas_orbit_native_posvel(jd_tdb, &orbit, pos, NULL));
  vector2radec(pos, &hm, NULL);

  return remainder(12.0 + hm - he, 24.0) * 15.0;
}

/**
 * Calculates the date / time at which the Moon will reach the specified phase next, _after_ the
 * specified time. It uses orbital models for Earth (E.M. Standish and J.G. Williams 1992), and
 * for the Moon (Chapront, J. et al., 2002), and takes into account the slightly eccentric nature
 * of both orbits.
 *
 * NOTES:
 * <ol>
 * <li>The Moon's phase here follows the definition by the Astronomical Almanac, as the excess
 * ecliptic longitude of the Moon over that of the Sun seen from the geocenter.</li>
 * <li>There are other definitions of the phase too, depending on which you might find slightly
 * different answers, but regardless of the details most phase calculations should match give or
 * take a few hours.</li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 *  <li>The Explanatory Supplement to the Astronomical Almanac, University Science Books, 3rd ed.,
 *      p. 507</li>
 *  <li>E.M. Standish and J.G. Williams 1992.</li>
 *  <li>https://ssd.jpl.nasa.gov/planets/approx_pos.html</li>
 *  <li>Chapront, J. et al., 2002, A&A 387, 700–709</li>
 *  <li>Chapront-Touze, M, and Chapront, J. 1983, Astronomy and Astrophysics (ISSN 0004-6361),
 *      vol. 124, no. 1, July 1983, p. 50-62.</li>
 * </ol>
 *
 * @param phase   [deg] The Moon's phase, or more precisely the ecliptic longitude difference
 *                between the Sun and the Moon, as seen from the geocenter. 0: New Moon, 90: 1st
 *                quarter, +/- 180 Full Moon, -90: 3rd quarter.
 * @param jd_tdb  [day] The lower bound date for the phase, as a Barycentric Dynamical Time (TDB)
 *                based Julian Date.
 * @return        [day] The Barycentric Dynamical Time (TDB) based Julian Date when the Moon will
 *                be in the desired phase next after the input date; or NAN if the solution failed
 *                to converge (errno will be set to ECANCELED).
 *
 * @since 1.4
 * @author Attila Kovacs
 *
 * @sa novas_moon_phase(), novas_make_moon_orbit()
 */
double novas_next_moon_phase(double phase, double jd_tdb) {
  static const char *fn = "novas_next_moon_phase";
  int i;

  for(i = 0; i < novas_inv_max_iter; i++) {
    double phi = novas_moon_phase(jd_tdb);
    double t = (jd_tdb - NOVAS_JD_J2000) / JULIAN_CENTURY_DAYS;

    // Differential motion of the Moon w.r.t. Earth.
    // Moon motion from Chapront-Touze, M, and Chapront, J. 1983, A&A, 124, 1, p. 50-62.
    // Earth motion from E.M. Standish and J.G. Williams 1992. Table 8.10.3. Valid for 3000 BC to 3000 AD.
    double rate = 445266.793243221 + t * (0.021258 + t * (3.75393e-05 - t * 2.366776e-07));
    rate /= JULIAN_CENTURY_DAYS;

    if(isnan(phi))
      return novas_trace_nan(fn);

    phi = remainder(phase - phi, DEG360);

    if(fabs(phi) < 1e-6)
      return jd_tdb;

    if(i == 0 && phi < 0.0)
      phi += DEG360;  // initial phase shift evolution must be positive to ensure it is after input date.

    // Date when mean elongation changes by phi...
    jd_tdb += phi / rate;
  }

  novas_error(-1, ECANCELED, fn, "Failed to converge");
  return NAN;
}

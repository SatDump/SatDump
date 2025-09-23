/**
 * @file
 *
 * @date Created  on Mar 5, 2025
 * @author Attila Kovacs
 *
 *  Various spectral / velocity related functions. __SuperNOVAS__ velocities are always calculated
 *  with relativistic corrections for motion (both source and observer), and gravitational effects
 *  (both at the source and at the observer), so they represent spectroscopically accurate values.
 *
 *  As such radial velocity measures are always specroscopically precise, and follow the relation:
 *  &lambda;<sub>obs</sub> / &lambda;<sub>rest</sub> = (1 + _z_) = ((1 + _v_<sub>rad</sub>/_c_) /
 *  (1 - _v_<sub>rad</sub>/_c_))<sup>1/2</sup>.
 */

#include <errno.h>

/// \cond PRIVATE
#define __NOVAS_INTERNAL_API__    ///< Use definitions meant for internal use by SuperNOVAS only
#include "novas.h"
/// \endcond

/// \cond PRIVATE
double novas_add_beta(double beta1, double beta2) {
  return (beta1 + beta2) / (1 + beta1 * beta2);
}

/**
 * Adds velocities defined in AU/day, using the relativistic formula.
 *
 * @param v1  [AU/day] First component
 * @param v2  [AU/day] Second component
 * @return    [AU/day] The relativistically coadded sum of the input velocities.
 *
 * @since 1.2
 * @author Attila Kovacs
 *
 * @sa novas_z_add()
 */
double novas_add_vel(double v1, double v2) {
  return novas_add_beta(v1 / C_AUDAY, v2 / C_AUDAY) * C_AUDAY;
}
/// \endcond

/// ==========================================================================

/**
 * Converts a redshift value (z = &delta;f / f<sub>rest</sub>) to a radial velocity (i.e. rate) of
 * recession. It is based on the relativistic formula:
 * <pre>
 *  1 + z = sqrt((1 + &beta;) / (1 - &beta;))
 * </pre>
 * where &beta; = v / c.
 *
 * @param z   the redshift value (&delta;&lambda; / &lambda;<sub>rest</sub>).
 * @return    [km/s] Corresponding velocity of recession, or NAN if the input redshift is invalid,
 *            i.e. z &lt;= -1).
 *
 * @author Attila Kovacs
 * @since 1.2
 *
 * @sa novas_v2z(), redshift_vrad()
 */
double novas_z2v(double z) {
  if(z <= -1.0) {
    novas_error(-1, ERANGE, "novas_z2v", "invalid redshift value z=%g", z);
    return NAN;
  }
  z += 1.0;
  z *= z;
  return (z - 1.0) / (z + 1.0) * C / NOVAS_KMS;
}

/**
 * Converts a radial recession velocity to a redshift value (z = &delta;f / f<sub>rest</sub>).
 * It is based on the relativistic formula:
 * <pre>
 *  1 + z = sqrt((1 + &beta;) / (1 - &beta;))
 * </pre>
 * where &beta; = v / c.
 *
 * @param vel   [km/s] velocity (i.e. rate) of recession.
 * @return      the corresponding redshift value (&delta;&lambda; / &lambda;<sub>rest</sub>), or
 *              NAN if the input velocity is invalid (i.e., it exceeds the speed of light).
 *
 * @author Attila Kovacs
 * @since 1.2
 *
 * @sa novas_z2v(), novas_z_add()
 */
double novas_v2z(double vel) {
  vel *= NOVAS_KMS / C;   // [km/s] -> beta
  if(fabs(vel) > 1.0) {
    novas_error(-1, EINVAL, "novas_v2z", "velocity exceeds speed of light v=%g km/s", vel * C / NOVAS_KMS);
    return NAN;
  }
  return sqrt((1.0 + vel) / (1.0 - vel)) - 1.0;
}

/**
 * Applies an incremental redshift correction to a radial velocity. For example, you may
 * use this function to correct a radial velocity calculated by `rad_vel()` or `rad_vel2()`
 * for a Solar-system body to account for the gravitational redshift for light originating
 * at a specific distance away from the body. For the Sun, you may want to undo the
 * redshift correction applied for the photosphere using `unredshift_vrad()` first.
 *
 * @param vrad    [km/s] Radial velocity
 * @param z       Redshift correction to apply
 * @return        [km/s] The redshift corrected radial velocity or NAN if the redshift value
 *                is invalid (errno will be set to EINVAL).
 *
 * @since 1.2
 * @author Attila Kovacs
 *
 * @sa unredshift_vrad(), grav_redshift(), novas_z_add()
 */
double redshift_vrad(double vrad, double z) {
  static const char *fn = "redshift_vrad";
  double z0;
  if(z <= -1.0) {
    novas_error(-1, EINVAL, fn, "invalid redshift value: z=%g", z);
    return NAN;
  }
  z0 = novas_v2z(vrad);
  if(isnan(z0)) novas_trace(fn, -1, 0);
  return novas_z2v((1.0 + z0) * (1.0 + z) - 1.0);
}

/**
 * Undoes an incremental redshift correction that was applied to radial velocity.
 *
 * @param vrad    [km/s] Radial velocity
 * @param z       Redshift correction to apply
 * @return        [km/s] The radial velocity without the redshift correction or NAN if the
 *                redshift value is invalid. (errno will be set to EINVAL)
 *
 * @since 1.2
 * @author Attila Kovacs
 *
 * @sa redshift_vrad(), grav_redshift()
 */
double unredshift_vrad(double vrad, double z) {
  static const char *fn = "unredshift_vrad";
  double z0;
  if(z <= -1.0) {
    novas_error(-1, EINVAL, fn, "invalid redshift value: z=%g", z);
    return NAN;
  }
  z0 = novas_v2z(vrad);
  if(isnan(z0)) novas_trace(fn, -1, 0);
  return novas_z2v((1.0 + z0) / (1.0 + z) - 1.0);
}

/**
 * Compounds two redshift corrections, e.g. to apply (or undo) a series gravitational redshift
 * corrections and/or corrections for a moving observer. It's effectively using
 * (1 + z) = (1 + z1) * (1 + z2).
 *
 * @param z1    One of the redshift values
 * @param z2    The other redshift value
 * @return      The compound redshift value, ot NAN if either input redshift is invalid (errno
 *              will be set to EINVAL).
 *
 * @since 1.2
 * @author Attila Kovacs
 *
 * @sa grav_redshift(), redshift_vrad(), unredshift_vrad()
 */
double novas_z_add(double z1, double z2) {
  if(z1 <= -1.0 || z2 <= -1.0) {
    novas_error(-1, EINVAL, "novas_z_add", "invalid redshift value: z1=%g, z2=%g", z1, z2);
    return NAN;
  }
  return z1 + z2 + z1 * z2;
}

/**
 * Returns the inverse of a redshift value, that is the redshift for a body moving with the same
 * velocity as the original but in the opposite direction.
 *
 * @param z     A redhift value
 * @return      The redshift value for a body moving in the opposite direction with the
 *              same speed, or NAN if the input redshift is invalid.
 *
 * @since 1.2
 * @author Attila Kovacs
 *
 * @sa novas_z_add()
 */
double novas_z_inv(double z) {
  if(z <= -1.0) {
    novas_error(-1, EINVAL, "novas_z_inv", "invalid redshift value: z=%g", z);
    return NAN;
  }
  return 1.0 / (1.0 + z) - 1.0;
}

static int convert_lsr_ssb_vel(const double *vLSR, int sign, double *vSSB) {
  static const double betaSSB[3] = { 11.1 * NOVAS_KMS / C, 12.24 * NOVAS_KMS / C, 7.25 * NOVAS_KMS / C };
  int i;

  for(i = 3; --i >= 0; )
    vSSB[i] = novas_add_beta(vLSR[i] * NOVAS_KMS / C, sign * betaSSB[i]) * C / NOVAS_KMS;

  return 0;
}

/**
 * Returns a Solar System Baricentric (SSB) radial velocity for a radial velocity that is
 * referenced to the Local Standard of Rest (LSR). Internally, NOVAS always uses barycentric
 * radial velocities, but it is just as common to have catalogs define radial velocities
 * referenced to the LSR.
 *
 * The SSB motion w.r.t. the barycenter is assumed to be (11.1, 12.24, 7.25) km/s in ICRS
 * (Shoenrich et al. 2010).
 *
 * REFERENCES:
 * <ol>
 * <li>Ralph Schoenrich, James Binney, Walter Dehnen, Monthly Notices of the Royal Astronomical
 * Society, Volume 403, Issue 4, April 2010, Pages 1829–1833,
 * https://doi.org/10.1111/j.1365-2966.2010.16253.x</li>
 * </ol>
 *
 * @param epoch     [yr] Coordinate epoch in which the coordinates and velocities are defined.
 *                  E.g. 2000.0.
 * @param ra        [h] Right-ascenscion of source at given epoch.
 * @param dec       [deg] Declination of source at given epoch.
 * @param vLSR      [km/s] radial velocity defined against the Local Standard of Rest (LSR), at
 *                  given epoch.
 *
 * @return          [km/s] Equivalent Solar-System Barycentric radial velocity.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_ssb_to_lsr_vel(), make_cat_entry()
 * @sa novas_str_hours(), novas_str_degrees()
 */
double novas_lsr_to_ssb_vel(double epoch, double ra, double dec, double vLSR) {
  double u[3] = {0.0}, v[3];
  double jd = NOVAS_JD_J2000 + 365.25 * (epoch - 2000.0);
  int i;

  radec2vector(ra, dec, 1.0, u);
  for(i = 3; --i >= 0; )
    v[i] = vLSR * u[i];

  precession(jd, v, NOVAS_JD_J2000, v);
  convert_lsr_ssb_vel(v, -1, v);
  precession(NOVAS_JD_J2000, v, jd, v);

  return novas_vdot(u, v);
}

/**
 * Returns a radial-velocity referenced to the Local Standard of Rest (LSR) for a given
 * Solar-System Barycentric (SSB) radial velocity. Internally, NOVAS always uses barycentric
 * radial velocities, but it is just as common to have catalogs define radial velocities
 * referenced to the LSR.
 *
 * The SSB motion w.r.t. the barycenter is assumed to be (11.1, 12.24, 7.25) km/s in ICRS
 * (Shoenrich et al. 2010).
 *
 * REFERENCES:
 * <ol>
 * <li>Ralph Schoenrich, James Binney, Walter Dehnen, Monthly Notices of the Royal Astronomical
 * Society, Volume 403, Issue 4, April 2010, Pages 1829–1833,
 * https://doi.org/10.1111/j.1365-2966.2010.16253.x</li>
 * </ol>
 *
 * @param epoch     [yr] Coordinate epoch in which the coordinates and velocities are defined.
 *                  E.g. 2000.0.
 * @param ra        [h] Right-ascenscion of source at given epoch.
 * @param dec       [deg] Declination of source at given epoch.
 * @param vLSR      [km/s] radial velocity defined against the Local Standard of Rest (LSR), at
 *                  given epoch.
 *
 * @return          [km/s] Equivalent Solar-System Barycentric radial velocity.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_lsr_to_ssb_vel(), make_cat_entry()
 * @sa novas_str_hours(), novas_str_degrees()
 */
double novas_ssb_to_lsr_vel(double epoch, double ra, double dec, double vLSR) {
  double u[3] = {0.0}, v[3];
  double jd = NOVAS_JD_J2000 + 365.25 * (epoch - 2000.0);
  int i;

  radec2vector(ra, dec, 1.0, u);
  for(i = 3; --i >= 0;)
    v[i] = vLSR * u[i];

  precession(jd, v, NOVAS_JD_J2000, v);
  convert_lsr_ssb_vel(v, 1, v);
  precession(NOVAS_JD_J2000, v, jd, v);

  return novas_vdot(u, v);
}

/**
 * Predicts the radial velocity of the observed object as it would be measured by spectroscopic
 * means.  Radial velocity is here defined as the radial velocity measure (z) times the speed of
 * light. For major planets (and Sun and Moon), it includes gravitational corrections for light
 * originating at the surface and observed from near Earth or else from a large distance away. For
 * other solar system bodies, it applies to a fictitious emitter at the center of the observed
 * object, assumed massless (no gravitational red shift). The corrections do not in general apply
 * to reflected light. For stars, it includes all effects, such as gravitational redshift,
 * contained in the catalog barycentric radial velocity measure, a scalar derived from
 * spectroscopy. Nearby stars with a known kinematic velocity vector (obtained independently of
 * spectroscopy) can be treated like solar system objects.
 *
 * Gravitational blueshift corrections for the Solar and Earth potential for observers are
 * included. However, the result does not include a blueshift correction for observers (e.g.
 * spacecraft) orbiting other major Solar-system bodies. You may adjust the amount of
 * gravitational redshift correction applied to the radial velocity via `redshift_vrad()`,
 * `unredshift_vrad()` and `grav_redshift()` if necessary.
 *
 * All the input arguments are BCRS quantities, expressed with respect to the ICRS axes. 'vel_src'
 * and 'vel_obs' are kinematic velocities - derived from geometry or dynamics, not spectroscopy.
 *
 * If the object is outside the solar system, the algorithm used will be consistent with the
 * IAU definition of stellar radial velocity, specifically, the barycentric radial velocity
 * measure, which is derived from spectroscopy.  In that case, the vector 'vel_src' can be very
 * approximate -- or, for distant stars or galaxies, zero -- as it will be used only for a small
 * geometric correction that is proportional to proper motion.
 *
 * Any of the distances (last three input arguments) can be set to zero (0.0) or negative if the
 * corresponding general relativistic gravitational potential term is not to be evaluated.
 * These terms generally are important at the meter/second level only. If 'd_obs_geo' and
 * 'd_obs_sun' are both zero, an average value will be used for the relativistic term for the
 * observer, appropriate for an observer on the surface of the Earth. 'd_src_sun', if given, is
 * used only for solar system objects.
 *
 * NOTES:
 * <ol>
 * <li>This function does not accont for the gravitational deflection of Solar-system sources.
 * For that purpose, the rad_vel2() function, introduced in v1.1, is more appropriate.</li>
 * <li>The NOVAS C implementation did not include relatistic corrections for a moving observer
 * if both `d_obs_geo` and `d_obs_sun` were zero. As of SuperNOVAS v1.1, the relatistic
 * corrections for a moving observer will be included in the radial velocity measure always.</li>
 * <li>In a departure from the original NOVAS C, the radial velocity for major planets (and Sun
 * and Moon) includes gravitational redshift corrections for light originating at the surface,
 * assuming it's observed from near Earth or else from a large distance away.</li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 * <li>Lindegren &amp; Dravins (2003), Astronomy &amp; Astrophysics 401, 1185-1201.</li>
 * <li>Unlike NOVAS C, this function will return a radial velocity for the Sun that is
 * gravitationally referenced to the Sun's photosphere. (NOVAS C returns the radial velocity for a
 * massless Sun)</li>
 * </ol>
 *
 * @param source        Celestial object observed
 * @param pos_src       [AU|*] Geometric position vector of object with respect to observer.
 *                      For solar system sources it should be corrected for light-time. For
 *                      non-solar-system objects, the position vector defines a direction only,
 *                      with arbitrary magnitude.
 * @param vel_src       [AU/day] Velocity vector of object with respect to solar system
 *                      barycenter.
 * @param vel_obs       [AU/day] Velocity vector of observer with respect to solar system
 *                      barycenter.
 * @param d_obs_geo     [AU] Distance from observer to geocenter, or &lt;=0.0 if
 *                      gravitational blueshifting due to Earth potential around observer can be
 *                      ignored.
 * @param d_obs_sun     [AU] Distance from observer to Sun, or &lt;=0.0 if gravitational
 *                      bluehifting due to Solar potential around observer can be ignored.
 * @param d_src_sun     [AU] Distance from object to Sun, or &lt;=0.0 if gravitational
 *                      redshifting due to Solar potential around source can be ignored.
 * @param[out] rv       [km/s] The observed radial velocity measure times the speed of light,
 *                      or NAN if there was an error.
 * @return              0 if successfule, or else -1 if there was an error (errno will be set
 *                      to EINVAL if any of the arguments are NULL, or to some other value to
 *                      indicate the type of error).
 *
 * @sa rad_vel2()
 *
 */
int rad_vel(const object *restrict source, const double *restrict pos_src, const double *vel_src, const double *vel_obs,
        double d_obs_geo, double d_obs_sun, double d_src_sun, double *restrict rv) {
  static const char *fn = "rad_vel";
  int stat;

  if(!rv)
    return novas_error(-1, EINVAL, fn, "NULL input source");

  *rv = rad_vel2(source, pos_src, vel_src, pos_src, vel_obs, d_obs_geo, d_obs_sun, d_src_sun);
  stat = isnan(*rv) ? -1 : 0;
  prop_error(fn, stat, 0);

  return 0;
}

/**
 * Predicts the radial velocity of the observed object as it would be measured by spectroscopic
 * means. This is a modified version of the original NOVAS C 3.1 rad_vel(), to account for
 * the different directions in which light is emitted vs in which it detected, e.g. when it is
 * gravitationally deflected.
 *
 * Radial velocity is here defined as the radial velocity measure (z) times the speed of light.
 * For major planets (and Sun and Moon), it includes gravitational corrections for light
 * originating at the surface and observed from near Earth or else from a large distance away. For
 * other solar system bodies, it applies to a fictitious emitter at the center of the observed
 * object, assumed massless (no gravitational red shift). The corrections do not in general apply
 * to reflected light. For stars, it includes all effects, such as gravitational redshift,
 * contained in the catalog barycentric radial velocity measure, a scalar derived from
 * spectroscopy. Nearby stars with a known kinematic velocity vector (obtained independently of
 * spectroscopy) can be treated like solar system objects.
 *
 * Gravitational blueshift corrections for the Solar and Earth potential for observers are
 * included. However, the result does not include a blueshift correction for observers (e.g.
 * spacecraft) orbiting other major Solar-system bodies. You may adjust the amount of
 * gravitational redshift correction applied to the radial velocity via `redshift_vrad()`,
 * `unredshift_vrad()` and `grav_redshift()` if necessary.
 *
 * All the input arguments are BCRS quantities, expressed with respect to the ICRS axes. 'vel_src'
 * and 'vel_obs' are kinematic velocities - derived from geometry or dynamics, not spectroscopy.
 *
 * If the object is outside the solar system, the algorithm used will be consistent with the
 * IAU definition of stellar radial velocity, specifically, the barycentric radial velocity
 * measure, which is derived from spectroscopy.  In that case, the vector 'vel_src' can be very
 * approximate -- or, for distant stars or galaxies, zero -- as it will be used only for a small
 * geometric and relativistic (time dilation) correction, including the proper motion.
 *
 * Any of the distances (last three input arguments) can be set to a negative value if the
 * corresponding general relativistic gravitational potential term is not to be evaluated.
 * These terms generally are important only at the meter/second level. If 'd_obs_geo' and
 * 'd_obs_sun' are both zero, an average value will be used for the relativistic term for the
 * observer, appropriate for an observer on the surface of the Earth. 'd_src_sun', if given, is
 * used only for solar system objects.
 *
 * NOTES:
 * <ol>
 * <li>This function is called by `novas_sky_pos()` and `place()` to calculate radial velocities
 * along with the apparent position of the source.</li>
 * <li>For major planets (and Sun and Moon), the radial velocity includes gravitational redshift
 * corrections for light originating at the surface, assuming it's observed from near Earth or
 * else from a large distance away.</li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 * <li>Lindegren &amp; Dravins (2003), Astronomy &amp; Astrophysics 401, 1185-1201.</li>
 * </ol>
 *
 * @param source        Celestial object observed
 * @param pos_emit      [AU|*] position vector of object with respect to observer in the
 *                      direction that light was emitted from the source.
 *                      For solar system sources it should be corrected for light-time. For
 *                      non-solar-system objects, the position vector defines a direction only,
 *                      with arbitrary magnitude.
 * @param vel_src       [AU/day] Velocity vector of object with respect to solar system
 *                      barycenter.
 * @param pos_det       [AU|*] apparent position vector of source, as seen by the observer.
 *                      It may be the same vector as `pos_emit`, in which case the routine
 *                      behaves like the original NOVAS_C rad_vel().
 * @param vel_obs       [AU/day] Velocity vector of observer with respect to solar system
 *                      barycenter.
 * @param d_obs_geo     [AU] Distance from observer to geocenter, or &lt;=0.0 if
 *                      gravitational blueshifting due to Earth potential around observer can be
 *                      ignored.
 * @param d_obs_sun     [AU] Distance from observer to Sun, or &lt;=0.0 if gravitational
 *                      bluehifting due to Solar potential around observer can be ignored.
 * @param d_src_sun     [AU] Distance from object to Sun, or &lt;=0.0 if gravitational
 *                      redshifting due to Solar potential around source can be ignored.
 *                      Additionally, a value &lt;0 will also skip corrections for light
 *                      originating at the surface of the observed major solar-system body.
 * @return              [km/s] The observed radial velocity measure times the speed of light,
 *                      or NAN if there was an error (errno will be set to EINVAL if any of the
 *                      arguments are NULL, or to some other value to indicate the type of error).
 *
 * @since 1.1
 * @author Attila Kovacs
 *
 * @sa rad_vel()
 * @sa novas_sky_pos(), novas_v2z()
 */
double rad_vel2(const object *restrict source, const double *pos_emit, const double *vel_src, const double *pos_det, const double *vel_obs,
        double d_obs_geo, double d_obs_sun, double d_src_sun) {
  static const char *fn = "rad_vel2";

  double rel; // redshift factor i.e., f_src / fobs = (1 + z)
  double uk[3], r, phi, beta_src, beta_obs, beta;
  int i;

  if(!source) {
    novas_set_errno(EINVAL, fn, "NULL input source");
    return NAN;
  }

  if(!pos_emit || !vel_src || !pos_det) {
    novas_set_errno(EINVAL, fn, "NULL input source pos/vel: pos_emit=%p, vel_src=%p, pos_det=%p", pos_emit, vel_src, pos_det);
    return NAN;
  }

  if(!vel_obs) {
    novas_set_errno(EINVAL, fn, "NULL input observer velocity");
    return NAN;
  }

  // Compute geopotential at observer, unless observer is within Earth.
  r = d_obs_geo * AU;
  phi = (r > 0.95 * NOVAS_EARTH_RADIUS) ? GE / r : 0.0;

  // Compute solar potential at observer unless well within the Sun
  r = d_obs_sun * AU;
  phi += (r > 0.95 * NOVAS_SOLAR_RADIUS) ? GS / r : 0.0;

  // Compute relativistic potential at observer.
  if(d_obs_geo == 0.0 && d_obs_sun == 0.0) {
    // Use average value for an observer on the surface of Earth
    // Lindegren & Dravins eq. (42), inverse.
    rel = 1.0 - 1.550e-8;
  }
  else {
    // Lindegren & Dravins eq. (41), second factor in parentheses.
    rel = 1.0 - phi / C2;
  }

  // Compute unit vector toward object (direction of emission).
  r = novas_vlen(pos_emit);
  for(i = 0; i < 3; i++)
    uk[i] = pos_emit[i] / r;

  // Complete radial velocity calculation.
  switch(source->type) {
    case NOVAS_CATALOG_OBJECT: {
      // Objects outside the solar system.
      // For stars, update barycentric radial velocity measure for change
      // in view angle.
      const cat_entry *star= &source->star;
      const double ra = star->ra * HOURANGLE;
      const double dec = star->dec * DEGREE;
      const double cosdec = cos(dec);

      // Compute radial velocity measure of sidereal source rel. barycenter
      // Including proper motion
      beta_src = star->radialvelocity * NOVAS_KMS / C;

      if(star->parallax > 0.0) {
        double du[3];

        du[0] = uk[0] - (cosdec * cos(ra));
        du[1] = uk[1] - (cosdec * sin(ra));
        du[2] = uk[2] - sin(dec);

        beta_src = novas_add_beta(beta_src, novas_vdot(vel_src, du) / C_AUDAY);
      }

      break;
    }

    case NOVAS_PLANET:
      if(d_src_sun >= 0.0) {
        // Gravitational potential for light originating at surface of major solar system body.
        const double zpl[NOVAS_PLANETS] = NOVAS_PLANET_GRAV_Z_INIT;
        if(source->number > 0 && source->number < NOVAS_PLANETS)
          rel *= (1.0 + zpl[source->number]);
      } // @suppress("No break at end of case")
      /* fallthrough */

    case NOVAS_EPHEM_OBJECT:
    case NOVAS_ORBITAL_OBJECT:
      // Solar potential at source (bodies strictly outside the Sun's volume)
      if(d_src_sun * AU > NOVAS_SOLAR_RADIUS)
        rel /= 1.0 - GS / (d_src_sun * AU) / C2;

      // Compute observed radial velocity measure of a planet rel. barycenter
      beta_src = novas_vdot(uk, vel_src) / C_AUDAY;

      break;

    default:
      novas_set_errno(EINVAL, fn, "invalid source type: %d", source->type);
      return NAN;
  }

  // Compute unit vector toward object (direction of detection).
  r = novas_vlen(pos_det);
  for(i = 0; i < 3; i++)
    uk[i] = pos_det[i] / r;

  // Radial velocity measure of observer rel. barycenter
  beta_obs = novas_vdot(uk, vel_obs) / C_AUDAY;

  // Differential barycentric radial velocity measure (relativistic formula)
  beta = novas_add_beta(beta_src, -beta_obs);

  // Include relativistic redhsift factor due to relative motion
  rel *= (1.0 + beta) / sqrt(1.0 - novas_vdist2(vel_obs, vel_src) / C2);

  // Convert observed radial velocity measure to kilometers/second.
  return novas_z2v(rel - 1.0);
}


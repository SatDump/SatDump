/**
 * @file
 *
 * @date Created  on Mar 6, 2025
 * @author Attila Kovacs and G. Kaplan
 *
 *  Various functions to deal with gravitational effects of the major Solar-system bodies on
 *  the astrometry.
 */

#include <string.h>
#include <errno.h>

/// \cond PRIVATE
#define __NOVAS_INTERNAL_API__    ///< Use definitions meant for internal use by SuperNOVAS only
#include "novas.h"
/// \endcond

// Defined in novas.h
int grav_bodies_reduced_accuracy = DEFAULT_GRAV_BODIES_REDUCED_ACCURACY;

// Defined in novas.h
int grav_bodies_full_accuracy = DEFAULT_GRAV_BODIES_FULL_ACCURACY;

/**
 * Computes the total gravitational deflection of light for the observed object due to the
 * major gravitating bodies in the solar system.  This function valid for an observed body
 * within the solar system as well as for a star.
 *
 * If 'accuracy' is NOVAS_FULL_ACCURACY (0), the deflections due to the Sun, Jupiter, Saturn,
 * and Earth are calculated.  Otherwise, only the deflection due to the Sun is calculated.
 * In either case, deflection for a given body is ignored if the observer is within ~1500 km
 * of the center of the gravitating body.
 *
 * NOTES:
 * <ol>
 * <li>This function is called by place() and `novas_sky_pos()` to calculate gravitational
 * deflections as appropriate for positioning sources precisely. The gravitational deflection due
 * to planets requires a planet calculator function to be configured, e.g.
 * via set_planet_provider().
 * </li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 * <li>Klioner, S. (2003), Astronomical Journal 125, 1580-1597, Section 6.</li>
 * </ol>
 *
 * @param jd_tdb      [day] Barycentric Dynamical Time (TDB) based Julian date
 * @param unused      The type of observer frame (no longer used)
 * @param accuracy    NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1). In full accuracy
 *                    mode, it will calculate the deflection due to the Sun, Jupiter, Saturn
 *                    and Earth. In reduced accuracy mode, only the deflection due to the
 *                    Sun is calculated.
 * @param pos_src     [AU] Position 3-vector of observed object, with respect to origin at
 *                    observer (or the geocenter), referred to ICRS axes, components
 *                    in AU.
 * @param pos_obs     [AU] Position 3-vector of observer (or the geocenter), with respect to
 *                    origin at solar system barycenter, referred to ICRS axes,
 *                    components in AU.
 * @param[out] out    [AU] Position vector of observed object, with respect to origin at
 *                    observer (or the geocenter), referred to ICRS axes, corrected
 *                    for gravitational deflection, components in AU. It can be the same
 *                    vector as the input, but not the same as pos_obs.
 * @return            0 if successful, -1 if any of the pointer arguments is NULL
 *                    or if the output vector is the same as pos_obs, or the error from
 *                    obs_planets().
 *
 * @sa grav_undef()
 * @sa novas_geom_to_app()
 * @sa novas_app_to_geom()
 * @sa set_planet_provider()
 * @sa set_planet_provider_hp()
 * @sa grav_bodies_full_accuracy
 * @sa grav_bodies_reduced_accuracy
 */
short grav_def(double jd_tdb, enum novas_observer_place unused, enum novas_accuracy accuracy, const double *pos_src, const double *pos_obs,
        double *out) {
  static const char *fn = "grav_def";

  novas_planet_bundle planets = {0};
  int pl_mask = (accuracy == NOVAS_FULL_ACCURACY) ? grav_bodies_full_accuracy : grav_bodies_reduced_accuracy;

  (void) unused;

  if(!pos_src || !out)
    return novas_error(-1, EINVAL, fn, "NULL source position 3-vector: pos_src=%p, out=%p", pos_src, out);

  prop_error(fn, obs_planets(jd_tdb, accuracy, pos_obs, pl_mask, &planets), 0);
  prop_error(fn, grav_planets(pos_src, pos_obs, &planets, out), 0);
  return 0;
}

/**
 * Corrects position vector for the deflection of light in the gravitational field of an
 * arbitrary body.  This function valid for an observed body within the solar system as
 * well as for a star.
 *
 * NOTES:
 * <ol>
 * <li>This function is called by grav_def() to calculate appropriate gravitational
 * deflections for sources.
 * </li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 * <li>Murray, C.A. (1981) Mon. Notices Royal Ast. Society 195, 639-648.</li>
 * <li>See also formulae in Section B of the Astronomical Almanac, or</li>
 * <li>Kaplan, G. et al. (1989) Astronomical Journal 97, 1197-1210, section iii f.</li>
 * </ol>
 *
 * @param pos_src   [AU] Position 3-vector of observed object, with respect to origin at
 *                  observer (or the geocenter), components in AU.
 * @param pos_obs   [AU] Position vector of gravitating body, with respect to origin
 *                  at solar system barycenter, components in AU.
 * @param pos_body  [AU] Position 3-vector of gravitating body, with respect to origin
 *                  at solar system barycenter, components in AU.
 * @param rmass     [1/Msun] Reciprocal mass of gravitating body in solar mass units, that
 *                  is, Sun mass / body mass.
 * @param[out] out  [AU]  Position 3-vector of observed object, with respect to origin at
 *                  observer (or the geocenter), corrected for gravitational
 *                  deflection, components in AU. It can the same vector as the input.
 * @return          0 if successful, or -1 if any of the input vectors is NULL.
 *
 * @sa grav_def()
 */
int grav_vec(const double *pos_src, const double *pos_obs, const double *pos_body, double rmass, double *out) {
  static const char *fn = "grav_vec";
  double pq[3], pe[3], pmag, emag, qmag, phat[3] = {0}, ehat[3], qhat[3];
  int i;

  if(!pos_src || !out)
    return novas_error(-1, EINVAL, fn, "NULL input or output 3-vector: pos_src=%p, out=%p", pos_src, out);

  // Default output in case of error
  if(out != pos_src)
    memcpy(out, pos_src, XYZ_VECTOR_SIZE);

  if(!pos_obs || !pos_body)
    return novas_error(-1, EINVAL, fn, "NULL input 3-vector: pos_obs=%p, pos_body=%p", pos_obs, pos_body);

  // Construct vector 'pq' from gravitating body to observed object and
  // construct vector 'pe' from gravitating body to observer.
  for(i = 3; --i >= 0;) {
    pe[i] = pos_obs[i] - pos_body[i];
    pq[i] = pe[i] + pos_src[i];
  }

  // Compute vector magnitudes and unit vectors.
  pmag = novas_vlen(pos_src);
  emag = novas_vlen(pe);
  qmag = novas_vlen(pq);

  // Gravitating body is the observer or the observed object. No deflection.
  if(!emag || !qmag)
    return 0;

  for(i = 3; --i >= 0;) {
    if(pmag)
      phat[i] = pos_src[i] / pmag;
    ehat[i] = pe[i] / emag;
    qhat[i] = pq[i] / qmag;
  }

  // Deflection calculation...
  {
    // Compute dot products of vectors
    const double edotp = novas_vdot(ehat, phat);
    const double pdotq = novas_vdot(phat, qhat);
    const double qdote = novas_vdot(qhat, ehat);

    // Compute scalar factors.
    const double fac1 = 2.0 * GS / (C * C * emag * AU * rmass);
    const double fac2 = 1.0 + qdote;

    // Construct corrected position vector 'pos2'.
    for(i = 3; --i >= 0;)
      out[i] += pmag * fac1 * (pdotq * ehat[i] - edotp * qhat[i]) / fac2;
  }

  return 0;
}

/**
 * Computes the gravitationally undeflected position of an observed source position due to the
 * specified Solar-system bodies.
 *
 * REFERENCES:
 * <ol>
 * <li>Klioner, S. (2003), Astronomical Journal 125, 1580-1597, Section 6.</li>
 * </ol>
 *
 * @param pos_app     [AU] Apparent position 3-vector of observed object, with respect to origin at
 *                    observer (or the geocenter), referred to ICRS axes, components
 *                    in AU.
 * @param pos_obs     [AU] Position 3-vector of observer (or the geocenter), with respect to
 *                    origin at solar system barycenter, referred to ICRS axes,
 *                    components in AU.
 * @param planets     Apparent planet data containing positions and velocities for the major
 *                    gravitating bodies in the solar-system.
 * @param[out] out    [AU] Nominal position vector of observed object, with respect to origin at
 *                    observer (or the geocenter), referred to ICRS axes, without gravitational
 *                    deflection, components in AU. It can be the same vector as the input, but not
 *                    the same as pos_obs.
 * @return            0 if successful, -1 if any of the pointer arguments is NULL.
 *
 * @sa obs_planets()
 * @sa grav_planets()
 * @sa novas_app_to_geom()
 *
 * @since 1.1
 * @author Attila Kovacs
 */
int grav_undo_planets(const double *pos_app, const double *pos_obs, const novas_planet_bundle *restrict planets, double *out) {
  static const char *fn = "grav_undo_planets";

  const double tol = 1e-13;
  double pos_def[3] = {0}, pos0[3] = {0};
  double l;
  int i;

  if(!pos_app || !pos_obs)
    return novas_error(-1, EINVAL, fn, "NULL input 3-vector: pos_app=%p, pos_obs=%p", pos_app, pos_obs);

  if(!planets)
    return novas_error(-1, EINVAL, fn, "NULL input planet data");

  if(!out)
    return novas_error(-1, EINVAL, fn, "NULL output 3-vector: out=%p", out);

  l = novas_vlen(pos_app);
  if(l == 0.0) {
    if(out != pos_app)
      memcpy(out, pos_app, XYZ_VECTOR_SIZE);
    return 0;        // Source is same as observer. No deflection.
  }

  memcpy(pos0, pos_app, sizeof(pos0));

  for(i = 0; i < novas_inv_max_iter; i++) {
    int j;

    prop_error(fn, grav_planets(pos0, pos_obs, planets, pos_def), 0);

    if(novas_vdist(pos_def, pos_app) / l < tol) {
      memcpy(out, pos0, sizeof(pos0));
      return 0;
    }

    for(j = 3; --j >= 0;)
      pos0[j] -= pos_def[j] - pos_app[j];
  }

  return novas_error(-1, ECANCELED, fn, "failed to converge");
}

/**
 * Computes the gravitationally undeflected position of an observed source position due to the
 * major gravitating bodies in the solar system.  This function valid for an observed body within
 * the solar system as well as for a star.
 *
 * If 'accuracy' is set to zero (full accuracy), three bodies (Sun, Jupiter, and Saturn) are
 * used in the calculation.  If the reduced-accuracy option is set, only the Sun is used in
 * the calculation.  In both cases, if the observer is not at the geocenter, the deflection
 * due to the Earth is included.
 *
 * The number of bodies used at full and reduced accuracy can be set by making a change to
 * the code in this function as indicated in the comments.
 *
 * REFERENCES:
 * <ol>
 * <li>Klioner, S. (2003), Astronomical Journal 125, 1580-1597, Section 6.</li>
 * </ol>
 *
 * @param jd_tdb      [day] Barycentric Dynamical Time (TDB) based Julian date
 * @param accuracy    NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param pos_app     [AU] Apparent position 3-vector of observed object, with respect to origin at
 *                    observer (or the geocenter), referred to ICRS axes, components
 *                    in AU.
 * @param pos_obs     [AU] Position 3-vector of observer (or the geocenter), with respect to
 *                    origin at solar system barycenter, referred to ICRS axes,
 *                    components in AU.
 * @param[out] out    [AU] Nominal position vector of observed object, with respect to origin at
 *                    observer (or the geocenter), referred to ICRS axes, without gravitational
 *                    deflection, components in AU. It can be the same vector as the input, but not
 *                    the same as pos_obs.
 * @return            0 if successful, -1 if any of the pointer arguments is NULL (errno = EINVAL)
 *                    or if the result did not converge (errno = ECANCELED), or else an error from
 *                    obs_planets().
 *
 * @sa grav_def()
 * @sa novas_app_to_geom()
 * @sa set_planet_provider()
 * @sa set_planet_provider_hp()
 * @sa grav_bodies_full_accuracy
 * @sa grav_bodies_reduced_accuracy
 *
 * @since 1.1
 * @author Attila Kovacs
 */
int grav_undef(double jd_tdb, enum novas_accuracy accuracy, const double *pos_app, const double *pos_obs, double *out) {
  static const char *fn = "grav_undef";

  novas_planet_bundle planets = {0};
  int pl_mask = (accuracy == NOVAS_FULL_ACCURACY) ? grav_bodies_full_accuracy : grav_bodies_reduced_accuracy;

  if(!pos_app || !out)
    return novas_error(-1, EINVAL, fn, "NULL source position 3-vector: pos_app=%p, out=%p", pos_app, out);

  prop_error(fn, obs_planets(jd_tdb, accuracy, pos_obs, pl_mask, &planets), 0);
  prop_error(fn, grav_undo_planets(pos_app, pos_obs, &planets, out), 0);
  return 0;
}

/**
 * Computes the total gravitational deflection of light for the observed object due to the
 * specified gravitating bodies in the solar system.  This function is valid for an observed body
 * within the solar system as well as for a star.
 *
 * NOTES:
 * <ol>
 * <li>The gravitational deflection due to planets requires a planet calculator function to be
 * configured, e.g. via set_planet_provider().</li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 * <li>Klioner, S. (2003), Astronomical Journal 125, 1580-1597, Section 6.</li>
 * </ol>
 *
 * @param pos_src     [AU] Position 3-vector of observed object, with respect to origin at
 *                    observer (or the geocenter), referred to ICRS axes, components
 *                    in AU.
 * @param pos_obs     [AU] Position 3-vector of observer (or the geocenter), with respect to
 *                    origin at solar system barycenter, referred to ICRS axes,
 *                    components in AU.
 * @param planets     Apparent planet data containing positions and velocities for the major
 *                    gravitating bodies in the solar-system.
 * @param[out] out    [AU] Position vector of observed object, with respect to origin at
 *                    observer (or the geocenter), referred to ICRS axes, corrected
 *                    for gravitational deflection, components in AU. It can be the same
 *                    vector as the input, but not the same as pos_obs.
 * @return            0 if successful, -1 if any of the pointer arguments is NULL
 *                    or if the output vector is the same as pos_obs.
 *
 * @sa obs_planets()
 * @sa grav_undo_planets()
 * @sa grav_def()
 * @sa novas_geom_to_app()
 * @sa grav_bodies_full_accuracy
 * @sa grav_bodies_reduced_accuracy
 *
 * @since 1.1
 * @author Attila Kovacs
 */
int grav_planets(const double *pos_src, const double *pos_obs, const novas_planet_bundle *restrict planets, double *out) {
  static const char *fn = "grav_planets";
  static const double rmass[] = NOVAS_RMASS_INIT;

  double tsrc;
  int i;

  if(!pos_src || !pos_obs)
    return novas_error(-1, EINVAL, fn, "NULL input 3-vector: pos_src=%p, pos_obs=%p", pos_src, pos_obs);

  if(!out)
    return novas_error(-1, EINVAL, fn, "NULL output 3-vector");

  if(!planets)
    return novas_error(-1, EINVAL, fn, "NULL input planet data");

  // Initialize output vector of observed object to equal input vector.
  if(out != pos_src)
    memcpy(out, pos_src, XYZ_VECTOR_SIZE);

  tsrc = novas_vlen(pos_src) / C_AUDAY;

  for(i = 1; i < NOVAS_PLANETS; i++) {
    double lt, dpl, p1[3];
    int k;

    if((planets->mask & (1 << i)) == 0)
      continue;

    // Distance from observer to gravitating body
    dpl = novas_vlen(&planets->pos[i][0]);

    // If observing from within ~1500 km of the gravitating body, then skip deflections by it...
    if(dpl < 1e-5)
      continue;

    // Calculate light time to the point where incoming geometric light ray is closest to gravitating body.
    lt = d_light(pos_src, &planets->pos[i][0]);

    // If gravitating body is in opposite direction from the source then use the gravitating
    // body position at the time the light is observed.
    if(lt < 0.0)
      lt = 0.0;

    // If source is between gravitating body and observer, then use gravitating body position
    // at the time light originated from source.
    else if(tsrc < lt)
      lt = tsrc;

    // Differential light time w.r.t. the apparent planet center
    lt -= dpl / C_AUDAY;

    // Calculate planet position at the time it is gravitationally acting on light.
    for(k = 3; --k >= 0;)
      p1[k] = pos_obs[k] + planets->pos[i][k] - lt * planets->vel[i][k];

    // Compute deflection due to gravitating body.
    grav_vec(out, pos_obs, p1, rmass[i], out);
  }

  return 0;
}

/**
 * Returns the gravitational redshift (_z_) for light emitted near a massive spherical body
 * at some distance from its center, and observed at some very large (infinite) distance away.
 *
 * @param M_kg    [kg] Mass of gravitating body that is contained inside the emitting radius.
 * @param r_m     [m] Radius at which light is emitted.
 * @return        The gravitational redshift (_z_) for an observer at very large  (infinite)
 *                distance from the gravitating body.
 *
 * @sa redshift_vrad()
 * @sa unredshift_vrad()
 * @sa novas_z_add()
 *
 * @since 1.2
 * @author Attila Kovacs
 */
double grav_redshift(double M_kg, double r_m) {
  static const double twoGoverC2 = 2.0 * 6.6743e-11 / (C * C); // 2G/c^2 in SI units.
  return 1.0 / sqrt(1.0 - twoGoverC2 * M_kg / r_m) - 1.0;
}


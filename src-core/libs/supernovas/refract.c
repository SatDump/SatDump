/**
 * @file
 *
 * @date Created  on Jun 27, 2024
 * @author Attila Kovacs and G. Kaplan
 *
 *  A collection of refraction models and utilities to use with novas_app_to_hor() or
 *  novas_hor_to_app().
 *
 * @sa novas_app_to_hor()
 * @sa novas_hor_to_app().
 */

/// \cond PRIVATE
#define __NOVAS_INTERNAL_API__      ///< Use definitions meant for internal use by SuperNOVAS only
/// \endcond

#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "novas.h"

#define NOVAS_DEFAULT_WAVELENGTH      0.55            ///< [&mu;m] Median wavelength of visible light.

static double lambda = NOVAS_DEFAULT_WAVELENGTH;      ///< [&mu;m] Observing wavelength


/**
 * Computes atmospheric optical refraction for a source at an astrometric zenith distance
 * (e.g. calculated without accounting for an atmosphere). This is suitable for converting
 * astrometric (unrefracted) zenith angles to observed (refracted) zenith angles. See
 * refract() for the reverse correction.
 *
 * The returned value is the approximate refraction for optical wavelengths. This function
 * can be used for planning observations or telescope pointing, but should not be used for
 * precise positioning.
 *
 * REFERENCES:
 * <ol>
 * <li>Explanatory Supplement to the Astronomical Almanac, p. 144.</li>
 * <li>Bennett, G. (1982), Journal of Navigation (Royal Institute) 35, pp. 255-259.</li>
 * </ol>
 *
 * @param location      Pointer to structure containing observer's location. It may also
 *                      contains weather data (optional) for the observer's location.
 *                      Some, but not all, refraction models will use location-based
 *                      (e.g. weather) information. For models that do not need it, it
 *                      may be NULL.
 * @param model         The built in refraction model to use. E.g. NOVAS_STANDARD_ATMOSPHERE (1),
 *                      or NOVAS_WEATHER_AT_LOCATION (2)...
 * @param zd_astro      [deg] Astrometric (unrefracted) zenith distance angle of the source.
 * @return              [deg] the calculated optical refraction. (to ~0.1 arcsec accuracy),
 *                      or 0.0 if the location is NULL or the option is invalid.
 *
 * @sa refract(), itrs_to_hor()
 *
 * @since 1.0
 * @author Attila Kovacs
 */
double refract_astro(const on_surface *restrict location, enum novas_refraction_model model, double zd_astro) {
  static const char *fn = "refract_astro";

  double refr = 0.0;
  int i;

  if(model == NOVAS_RADIO_REFRACTION)
    return novas_radio_refraction(0.0, location, NOVAS_REFRACT_ASTROMETRIC, 90.0 - zd_astro);

  errno = 0;

  for(i = 0; i < novas_inv_max_iter; i++) {
    double zd_obs = zd_astro - refr;
    refr = refract(location, model, zd_obs);
    if(errno)
      return novas_trace_nan(fn);
    if(fabs(refr - (zd_astro - zd_obs)) < 3.0e-5)
      return refr;
  }

  novas_set_errno(ECANCELED, fn, "failed to converge");
  return NAN;
}

/**
 * Computes atmospheric optical refraction for an observed (already refracted!) zenith
 * distance through the atmosphere. In other words this is suitable to convert refracted
 * zenith angles to astrometric (unrefracted) zenith angles. For the reverse, see
 * refract_astro().
 *
 * The returned value is the approximate refraction for optical wavelengths. This function
 * can be used for planning observations or telescope pointing, but should not be used for
 * precise positioning.
 *
 * NOTES:
 * <ol>
 * <li>The standard temeperature model includes a very rough estimate of the mean annual
 * temeprature for the ovserver's latitude and elevation, rather than the 10 C everywhere
 * assumption in NOVAS C 3.1.<.li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 * <li>Explanatory Supplement to the Astronomical Almanac, p. 144.</li>
 * <li>Bennett, G. (1982), Journal of Navigation (Royal Institute) 35, pp. 255-259.</li>
 * </ol>
 *
 * @param location      Pointer to structure containing observer's location. It may also
 *                      contains weather data (optional) for the observer's location.
 *                      Some, but not all, refraction models will use location-based
 *                      (e.g. weather) information. For models that do not need it, it
 *                      may be NULL.
 * @param model         The built in refraction model to use. E.g. NOVAS_STANDARD_ATMOSPHERE (1),
 *                      or NOVAS_WEATHER_AT_LOCATION (2)...
 * @param zd_obs        [deg] Observed (already refracted!) zenith distance through the
 *                      atmosphere.
 * @return              [deg] the calculated optical refraction or 0.0 if the location is
 *                      NULL or the option is invalid or the 'zd_obs' is invalid (&lt;90&deg;).
 *
 * @sa refract_astro(), hor_to_itrs()
 */
double refract(const on_surface *restrict location, enum novas_refraction_model model, double zd_obs) {
  static const char *fn = "refract";

  double p, t, h, r;

  if(model < 0 || model >= NOVAS_REFRACTION_MODELS) {
    novas_set_errno(EINVAL, fn, "invalid refraction model option: %d", model);
    return 0.0;
  }

  if(model == NOVAS_NO_ATMOSPHERE)
    return 0.0;

  if(!location) {
    novas_set_errno(EINVAL, fn, "NULL observer location");
    return 0.0;
  }

  if(location->temperature < -150.0 || location->temperature > 100.0) {
    novas_set_errno(EINVAL, fn, "invalid temperature value: %g C", location->temperature);
    return NAN;
  }

  if(location->pressure < 0.0 || location->pressure > 2000.0) {
    novas_set_errno(EINVAL, fn, "invalid pressure value: %g mbar", location->pressure);
    return NAN;
  }

  zd_obs = fabs(zd_obs);

  // Compute refraction up to zenith distance 91 degrees.
  if(zd_obs > 91.0)
    return 0.0;

  if(model == NOVAS_RADIO_REFRACTION)
    return novas_radio_refraction(0.0, location, NOVAS_REFRACT_OBSERVED, 90.0 - zd_obs);

  if(model == NOVAS_WAVE_REFRACTION)
    return novas_wave_refraction(0.0, location, NOVAS_REFRACT_OBSERVED, 90.0 - zd_obs);

  // If observed weather data are available, use them.  Otherwise, use
  // crude estimates of average conditions.
  if(model == NOVAS_WEATHER_AT_LOCATION) {
    p = location->pressure;
    t = location->temperature;
  }
  else {
    on_surface site = *location;
    novas_set_default_weather(&site);
    p = site.pressure;
    t = site.temperature;
  }

  h = 90.0 - zd_obs;
  r = 0.016667 / tan((h + 7.31 / (h + 4.4)) * DEGREE);
  return r * (0.28 * p / (t + 273.0));
}

static double novas_refraction(enum novas_refraction_model model, const on_surface *loc, enum novas_refraction_type type, double el) {
  static const char *fn = "novas_refraction";

  if(!loc) {
    novas_set_errno(EINVAL, fn, "NULL on surface observer location");
    return NAN;
  }

  if(type == NOVAS_REFRACT_OBSERVED)
    return refract(loc, model, 90.0 - el);

  if(type == NOVAS_REFRACT_ASTROMETRIC)
    return refract_astro(loc, model, 90.0 - el);

  novas_set_errno(EINVAL, fn, "NULL on surface observer location");
  return NAN;
}

/**
 * Computes the reverse atmospheric refraction for a given refraction model. Thus if a refraction
 * model takes observed elevation as an input, the reverse refraction takes astrometric elevation
 * as its input, and vice versa.
 *
 * @param model     The original refraction model
 * @param jd_tt     [day] Terrestrial Time (TT) based Julian data of observation
 * @param loc       Pointer to structure defining the observer's location on earth, and local
 *                  weather
 * @param type      Refraction type to use for the original model: NOVAS_REFRACT_OBSERVED (-1) or
 *                  NOVAS_REFRACT_ASTROMETRIC (0).
 * @param el0       [deg] input elevation for the inverse refraction model.
 * @return          [deg] Estimated refraction, or NAN if there was an error (it should also
 *                  set errno to indicate the type of error).
 *
 * @sa refract_astro(), itrs_to_hor()
 *
 * @since 1.1
 * @author Attila Kovacs
 */
double novas_inv_refract(RefractionModel model, double jd_tt, const on_surface *restrict loc, enum novas_refraction_type type, double el0) {
  static const char *fn = "novas_inv_refract";

  double refr = 0.0;
  const int dir = (type == NOVAS_REFRACT_OBSERVED ? 1 : -1);
  int i;

  errno = 0;

  for(i = 0; i < novas_inv_max_iter; i++) {
    double el1 = el0 + dir * refr;
    refr = model(jd_tt, loc, type, el1);
    if(errno)
      return novas_trace_nan(fn);

    if(fabs(refr - dir * (el1 - el0)) < 1e-7)
      return refr;
  }

  novas_set_errno(ECANCELED, fn, "failed to converge");
  return NAN;
}

/**
 * Returns an optical refraction correction for a standard atmosphere.
 *
 * @param jd_tt     [day] Terrestrial Time (TT) based Julian data of observation (unused in this
 *                  implementation of RefractionModel)
 * @param loc       Pointer to structure defining the observer's location on earth, and local
 *                  weather
 * @param type      Whether the input elevation is observed or astrometric: NOVAS_REFRACT_OBSERVED
 *                  (-1) or NOVAS_REFRACT_ASTROMETRIC (0).
 * @param el        [deg] Astrometric (unrefracted) source elevation
 * @return          [deg] Estimated refraction, or NAN if there was an error (it should also
 *                  set errno to indicate the type of error).
 *
 * @sa novas_optical_refraction(), novas_radio_refraction(), novas_wave_refraction()
 * @sa refract(), refract_astro()
 */
double novas_standard_refraction(double jd_tt, const on_surface *loc, enum novas_refraction_type type, double el) {
  double dz = novas_refraction(NOVAS_STANDARD_ATMOSPHERE, loc, type, el);
  (void) jd_tt;

  if(isnan(dz))
    return novas_trace_nan("novas_standard_refraction");
  return dz;
}

/**
 * Returns an optical refraction correction using the weather parameters defined for the observer
 * location. As such, make sure that temperature and pressure are defined, e.g. set after
 * calling e.g. `make_gps_site()`, `make_itrf_site(), `make_xyz_site()`, or similar call that
 * initializes the observing site.
 *
 * @param jd_tt     [day] Terrestrial Time (TT) based Julian data of observation (unused in this
 *                  implementation of RefractionModel)
 * @param loc       Pointer to structure defining the observer's location on earth, and local
 *                  weather values (temperature and pressure are used by this call).
 * @param type      Whether the input elevation is observed or astrometric: NOVAS_REFRACT_OBSERVED
 *                  (-1) or NOVAS_REFRACT_ASTROMETRIC (0).
 * @param el        [deg] Astrometric (unrefracted) source elevation
 * @return          [arcsec] Estimated refraction, or NAN if there was an error (it should also
 *                  set errno to indicate the type of error).
 *
 * @sa novas_wave_refraction(), novas_radio_refraction(), novas_standard_refraction()
 * @sa refract(), refract_astro()
 */
double novas_optical_refraction(double jd_tt, const on_surface *loc, enum novas_refraction_type type, double el) {
  double dz = novas_refraction(NOVAS_WEATHER_AT_LOCATION, loc, type, el);
  (void) jd_tt;

  if(isnan(dz))
    return novas_trace_nan("novas_optical_refraction");
  return dz;
}

/**
 * Atmospheric refraction model for radio wavelengths (Berman &amp; Rockwell 1976).
 *
 *
 * It uses the weather parameters defined for the location, including humidity. As such, make sure
 * the weather data is fully defined, and that the humidity was explicitly set after calling e.g.
 * `make_gps_site()`, `make_itrf_site(), `make_xyz_site()`, or similar call that initializes the
 * observing site.
 *
 * Adapted from FORTAN code provided by Berman &amp; Rockwell 1976.
 *
 * REFERENCES:
 * <ol>
 * <li>Berman, Allan L., and Rockwell, Stephen T. (1976), NASA JPL Technical Report 32-1601</li>
 * </ol>
 *
 * @param jd_tt     [day] Terrestrial Time (TT) based Julian data of observation (unused in this
 *                  implementation of RefractionModel)
 * @param loc       Pointer to structure defining the observer's location on earth, and local
 *                  weather. Make sure all weather values, including humidity (added in v1.1), are
 *                  fully populated.
 * @param type      Whether the input elevation is observed or astrometric: NOVAS_REFRACT_OBSERVED
 *                  (-1) or NOVAS_REFRACT_ASTROMETRIC (0).
 * @param el        [deg] source elevation of the specified type.
 * @return          [deg] Estimated refraction, or NAN if there was an error (it should also
 *                  set errno to indicate the type of error). An error is returned if the location
 *                  is NULL, or if the weather parameters are way outside of their resonable
 *                  ranges, or if the elevation is outside the supported [-1:90] range.
 *
 * @sa novas_optical_refraction(), novas_wave_refraction(), novas_standard_refraction()
 * @sa make_itrf_site(), make_gps_site(), make_xyz_site(), make_itrf_observer(), make_gps_observer()
 */
double novas_radio_refraction(double jd_tt, const on_surface *loc, enum novas_refraction_type type, double el) {
  static const char *fn = "novas_radio_refraction";
  // Various coefficients...
  static const double E[] = { 0.0, 46.625, 45.375, 4.1572, 1.4468, 0.25391, 2.2716, -1.3465, -4.3877, 3.1484, 4.520, -1.8982, 0.8900 };

  double E0, TK;
  double y, z;
  double poly;
  double fptem;
  double refraction;
  int j;

  (void) jd_tt;

  if(!loc) {
    novas_set_errno(EINVAL, fn, "NULL on surface observer location");
    return NAN;
  }

  if(loc->temperature < -150.0 || loc->temperature > 100.0) {
    novas_set_errno(EINVAL, fn, "invalid temperature value: %g C", loc->temperature);
    return NAN;
  }

  if(loc->pressure < 0.0 || loc->pressure > 2000.0) {
    novas_set_errno(EINVAL, fn, "invalid pressure value: %g mbar", loc->pressure);
    return NAN;
  }

  if(loc->humidity < 0.0 || loc->humidity > 100.0) {
    novas_set_errno(EINVAL, fn, "invalid humidity value: %g", loc->humidity);
    return NAN;
  }

  if(type == NOVAS_REFRACT_OBSERVED)
    return novas_inv_refract(novas_radio_refraction, jd_tt, loc, NOVAS_REFRACT_ASTROMETRIC, el);

  if(type != NOVAS_REFRACT_ASTROMETRIC) {
    novas_set_errno(EINVAL, fn, "invalid refraction type: %d", type);
    return NAN;
  }

  if(el < -1.0 || el > 90.0) {
    novas_set_errno(EINVAL, fn, "invalid input elevation: %g deg", el);
    return NAN;
  }

  // Zenith angle in degrees
  z = 90.0 - el;

  // Temperature in Kelvin
  TK = loc->temperature + 273.16;
  fptem = (loc->pressure / 1000.) * (273.16 / TK);
  E0 = (z - E[1]) / E[2];
  poly = E[11];

  for(j = 1; j <= 8; j++)
    poly = poly * E0 + E[11 - j];

  // Not needed... (?)
  //if(poly <= -80.)
  //  poly = 0.0;

  poly = exp(poly) - E[12];
  refraction = poly * fptem / 3600.0;
  y = exp(((TK * 17.149) - 4684.1) / (TK - 38.45));

  return refraction * (1.0 + (y * loc->humidity * 71.) / (TK * loc->pressure * 0.760));
}

/**
 * Sets the observing wavelength for which refraction is to be calculated when using a
 * wavelength-depenendent model, such as novas_wave_refraction().
 *
 * @param microns     [&mu;m] Observed wavelength to assume in refraction calculations
 * @return            0 if successful, or else -1 (errno set to `EINVAL`) if the wavelength
 *                    invalid (zero, negative, or NaN).
 *
 * @since 1.4
 * @author Attila Kovacs
 *
 * @sa novas_wave_refraction(), NOVAS_DEFAULT_WAVELENGTH
 */
int novas_refract_wavelength(double microns) {
  if(microns <= 0.0 || microns != microns)
    return novas_error(-1, EINVAL, "novas_refract_wavelength", "invalid wavelength: %f microns", microns);
  lambda = microns;
  return 0;
}

/**
 * The wavelength-dependent IAU atmospheric refraction model, based on the SOFA `iauRefco()`
 * function, in compliance to the 'SOFA Software License' terms of the original source. Our
 * implementation is not provided nor it is endorsed by SOFA. The original function has been
 * modified slightly, such as:
 *
 * <ol>
 * <li>Out-of-range weather parameters will return with an error (`errno` set to `EINVAL`), unlike
 * the SOFA implementation, which sets minimal or maximal allowed values for these.</li>
 * <li>The algorithm has been simplified to use fewer variables and simpler logic.</li>
 * <li>The SOFA function this implementation is based on returns A/B coefficients, whereas this
 * implementation returns the refraction correction angle.</li>
 * </ol>
 *
 * The refraction is calculated for the observing wavelenth previously set via
 * `novas_refract_wavelength()`, or for visible light at 550 nm by default.
 *
 * The function uses the weather parameters defined for the location, including humidity. As such,
 * make sure the weather data is fully defined, and that the humidity was explicitly set after
 * calling e.g. `make_gps_site()`, `make_itrf_site(), `make_xyz_site()`, or similar call that
 * initializes the observing site.
 *
 * According to the documentation of SOFA's `iauRefco()` function, the model has the following
 * accuracy for elevation angles between 15 and 75 degrees, under a range of typical surface
 * conditions:
 *
 *   |                  |    worst   |     RMS    |
 *   |------------------|:----------:|:----------:|
 *   | optical/IR       |   62 mas   |   8 mas    |
 *   | radio            |  319 mas   |  49 mas    |
 *
 *
 * NOTES:
 * <ol>
 * <li>
 * From the SOFA documentation: "The model balances speed and accuracy to give good results in
 * applications where performance at low altitudes is not paramount. Performance is maintained
 * across a range of conditions, and applies to both optical/IR and radio."
 * </li>
 * <li>The model is divergent in the observed direction of the horizon. As such, it should not be
 * used for calculating refraction at or below the horizon itself.</li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 * <li>
 *     Crane, R.K., Meeks, M.L. (ed), "Refraction Effects in the Neutral
 *     Atmosphere", Methods of Experimental Physics: Astrophysics 12B,
 *     Academic Press, 1976.
 * </li>
 * <li>
 *     Gill, Adrian E., "Atmosphere-Ocean Dynamics", Academic Press,
 *     1982.
 * </li>
 * <li>
 *     Green, R.M., "Spherical Astronomy", Cambridge University Press,
 *     1987.
 * </li>
 * <li>
 *     Hohenkerk, C.Y., &amp; Sinclair, A.T., NAO Technical Note No. 63,
 *     1985.
 * </li>
 * <li>
 *     Rueger, J.M., "Refractive Index Formulae for Electronic Distance
 *     Measurement with Radio and Millimetre Waves", in Unisurv Report
 *     S-68, School of Surveying and Spatial Information Systems,
 *     University of New South Wales, Sydney, Australia, 2002.
 * </li>
 * <li>
 *     Stone, Ronald C., P.A.S.P. 108, 1051-1058, 1996.
 * </li>
 * </ol>
 *
 * @param jd_tt     [day] Terrestrial Time (TT) based Julian data of observation (unused in this
 *                  implementation of RefractionModel)
 * @param loc       Pointer to structure defining the observer's location on earth, and local
 *                  weather. Make sure all weather values, including humidity (added in v1.1), are fully
 *                  populated.
 * @param type      Whether the input elevation is observed or astrometric: NOVAS_REFRACT_OBSERVED
 *                  (-1) or NOVAS_REFRACT_ASTROMETRIC (0).
 * @param el        [deg] observed source elevation of the specified type.
 * @return          [deg] Estimated refraction, or NAN if there was an error (it should also
 *                  set errno to indicate the type of error), e.g. because the location is NULL, or
 *                  because the weather parameters are outside of the supported (sensible) range, or
 *                  because the elevation is outside of the supported (0:90] range, or because the
 *                  wavelength set is below 100 nm (0.1 &mu;m);
 *
 * @since 1.4
 * @author Attila Kovacs
 *
 * @sa novas_refract_wavelength(), novas_optical_refraction(), novas_radio_refraction()
 * @sa make_gps_site(), make_itrf_site(), make_xyz_site(), make_gps_observer(), make_itrf_observer()
 */
double novas_wave_refraction(double jd_tt, const on_surface *loc, enum novas_refraction_type type, double el) {
  static const char *fn = "novas_wave_refration";

  double p, t, r, ps = 0.0, pw = 0.0, gamma, beta, a, b, tanz;

  if(!loc) {
    novas_set_errno(EINVAL, fn, "NULL on surface observer location");
    return NAN;
  }

  if(loc->temperature < -150.0 || loc->temperature > 200.0) {
    novas_set_errno(EINVAL, fn, "invalid temperature value: %g C", loc->temperature);
    return NAN;
  }

  if(loc->pressure < 0.0 || loc->pressure > 10000.0) {
    novas_set_errno(EINVAL, fn, "invalid pressure value: %g mbar", loc->pressure);
    return NAN;
  }

  if(loc->humidity < 0.0 || loc->humidity > 100.0) {
    novas_set_errno(EINVAL, fn, "invalid humidity value: %g %%", loc->humidity);
    return NAN;
  }

  if(lambda < 0.1) {
    novas_set_errno(EINVAL, fn, "wavelength too low: %g mirons", lambda);
    return NAN;
  }

  if(type == NOVAS_REFRACT_ASTROMETRIC)
    return novas_inv_refract(novas_wave_refraction, jd_tt, loc, NOVAS_REFRACT_OBSERVED, el);

  if(type != NOVAS_REFRACT_OBSERVED) {
    novas_set_errno(EINVAL, fn, "invalid refraction type: %d", type);
    return NAN;
  }

  if(el <= 0.0 || el > 90.0) {
    novas_set_errno(EINVAL, fn, "invalid input elevation: %g deg", el);
    return NAN;
  }

  (void) jd_tt; // unused

  t = loc->temperature;
  p = loc->pressure;
  r = 0.01 * loc->humidity;

  // Water vapour pressure at the observer.
  ps = pow(10.0, (0.7859 + 0.03477 * t) / (1.0 + 0.00412 * t)) * (1.0 + p * (4.5e-6 + 6e-10 * t * t));
  pw = r * ps / (1.0 - (1.0 - r) * ps / p);

  t += 273.15;    // C -> K

  // Formula for beta from Stone, with empirical adjustments.
  beta = 4.4474e-6 * t;

  if(lambda <= 100.0) {
    // Optical/IR refraction
    double w2 = lambda * lambda;
    gamma = ((77.53484e-6 + (4.39108e-7 + 3.666e-9 / w2) / w2) * p - 11.2684e-6 * pw) / t;
  }
  else {
    // Radio refraction
    gamma = (77.6890e-6 * p - (6.3938e-6 - 0.375463 / t) * pw) / t;
    beta -= 0.0074 * pw * beta;
  }

  // Refraction constants from Green.
  a = gamma * (1.0 - beta);
  b = -gamma * (beta - gamma / 2.0);

  tanz = tan((90.0 - el) * DEGREE);

  return tanz * (a + b * tanz * tanz) / DEGREE;
}

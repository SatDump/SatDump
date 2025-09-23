/**
 * @file
 *
 * @date Created  on Mar 6, 2025
 * @author Attila Kovacs
 *
 *  Functions that allow to define or access user-defined plugin routines.
 */

#include <errno.h>

/// \cond PRIVATE
#define __NOVAS_INTERNAL_API__    ///< Use definitions meant for internal use by SuperNOVAS only
#include "novas.h"
/// \endcond

/// \cond PRIVATE
#ifndef DEFAULT_SOLSYS
/// Will use solarsystem() and solarsystem_hp() that is linked with application
#  define DEFAULT_SOLSYS    0
#endif

// <---------- GLOBAL VARIABLES -------------->

#if DEFAULT_SOLSYS <= 0
static short solarsystem_adapter(double jd_tdb, enum novas_planet body, enum novas_origin origin,
        double *restrict position, double *restrict velocity) {
  solarsystem(jd_tdb, (short) body, (short) origin, position, velocity);
}

static short solarsystem_hp_adapter(const double jd_tdb[2], enum novas_planet body, enum novas_origin origin,
        double *restrict position, double *restrict velocity) {
  solarsystem_hp(jd_tdb, (short) body, (short) origin, position, velocity);
}

novas_planet_provider planet_call = solarsystem_adapter;
novas_planet_provider_hp planet_call_hp = solarsystem_hp_adapter;
#endif
/// \endcond


/// function to use for reading ephemeris data for all types of solar system sources
static novas_ephem_provider readeph2_call = NULL;

/// Function to use for reduced-precision calculations. (The full IAU 2000A model is used
/// always for high-precision calculations)
static novas_nutation_provider nutate_lp = iau2000b;


/**
 * Sets the function to use for obtaining position / velocity information for minor planets,
 * or sattelites.
 *
 * @param func   new function to use for accessing ephemeris data for minor planets or satellites.
 * @return       0 if successful, or else -1 if the function argument is NULL.
 *
 * @since 1.0
 * @author Attila Kovacs
 *
 * @sa get_ephem_provider(), set_planet_provider(), set_planet_provider_hp()
 * @sa novas_use_calceph(), novas_use_cspice()
 */
int set_ephem_provider(novas_ephem_provider func) {
  readeph2_call = func;
  return 0;
}

/**
 * Returns the user-defined ephemeris accessor function.
 *
 * @return    the currently defined function for accessing ephemeris data for minor planets
 *            or satellites, ot NULL if no function was set via set_ephem_provider() previously.
 *
 * @since 1.0
 * @author Attila Kovacs
 *
 * @sa set_ephem_provider(), get_planet_provider(), get_planet_provider_hp(), ephemeris()
 */
novas_ephem_provider get_ephem_provider() {
  return readeph2_call;
}

/**
 * Set the function to use for low-precision IAU 2000 nutation calculations instead of the
 * default nu2000k().
 *
 * @param func  the new function to use for low-precision IAU 2000 nutation calculations
 * @return      0 if successful, or -1 if the input argument is NULL
 *
 * @since 1.0
 * @author Attila Kovacs
 *
 * @sa get_nutation_lp_provider(), nutation_angles(), nutation()
 */
int set_nutation_lp_provider(novas_nutation_provider func) {
  if(!func)
    return novas_error(-1, EINVAL, "set_nutation_lp_provider", "NULL 'func' parameter");

  nutate_lp = func;
  return 0;
}

/**
 * Returns the function configured for low-precision IAU 2000 nutation calculations instead
 * of the default nu2000k().
 *
 * @return   the function to use for low-precision IAU 2000 nutation calculations
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa set_nutation_lp_provider(), nutation_angles(), nutation()
 */
novas_nutation_provider get_nutation_lp_provider() {
  return nutate_lp;
}

/**
 * Set a custom function to use for regular precision (see NOVAS_REDUCED_ACCURACY)
 * ephemeris calculations instead of the default solarsystem() routine.
 *
 * @param func    The function to use for solar system position/velocity calculations.
 *                See solarsystem() for further details on what is required of this
 *                function.
 *
 * @author Attila Kovacs
 * @since 1.0
 *
 * @sa get_planet_provider(), set_planet_provider_hp(), solarsystem(), NOVAS_REDUCED_ACCURACY
 * @sa novas_use_calceph(), novas_use_cspice()
 */
int set_planet_provider(novas_planet_provider func) {
  if(!func)
    return novas_error(-1, EINVAL, "set_planet_provider", "NULL 'func' parameter");

  planet_call = func;
  return 0;
}

/**
 * Returns the custom (low-precision) ephemeris provider function for major planets
 * (and Sun, Moon, SSB...), if any.
 *
 * @return    the custom (low-precision) planet ephemeris provider function.
 *
 * @author Attila Kovacs
 * @since 1.2
 *
 * @sa set_planet_provider(), get_planet_provider_hp(), get_ephem_provider()
 */
novas_planet_provider get_planet_provider() {
  return planet_call;
}

/**
 * Set a custom function to use for high precision (see NOVAS_FULL_ACCURACY) ephemeris
 * calculations instead of the default solarsystem_hp() routine.
 *
 * @param func    The function to use for solar system position/velocity calculations.
 *                See solarsystem_hp() for further details on what is required of this
 *                function.
 *
 * @author Attila Kovacs
 * @since 1.0
 *
 * @sa get_planet_provider_hp(), set_planet_provider(), solarsystem_hp(), NOVAS_FULL_ACCURACY
 * @sa novas_use_calceph(), novas_use_cspice()
 */
int set_planet_provider_hp(novas_planet_provider_hp func) {
  if(!func)
    return novas_error(-1, EINVAL, "set_planet_provider_hp", "NULL 'func' parameter");

  planet_call_hp = func;
  return 0;
}

/**
 * Returns the custom high-precision ephemeris provider function for major planets
 * (and Sun, Moon, SSB...), if any.
 *
 * @return    the custom high-precision planet ephemeris provider function.
 *
 * @author Attila Kovacs
 * @since 1.2
 *
 * @sa set_planet_provider_hp(), get_planet_provider(), get_ephem_provider()
 */
novas_planet_provider_hp get_planet_provider_hp() {
  return planet_call_hp;
}


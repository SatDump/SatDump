/**
 * @file
 *
 * @date Created  on Nov 6, 2024
 * @author Attila Kovacs
 *
 *  Solar system ID mappings between NOVAS and NASA's Navigation and Ancillary Information
 *  Facility (NAIF), which is used by the JPL ephemeris systems. The two differ for the numbering
 *  convention for major planets, the Sun, Moon, Solar-System Barycenter (SSB) and the Earth-Moon
 *  Barycenter (EMB). NOVAS does not have predefined IDs beyond this set (and no defined ID for
 *  EMB), thus for all other objects we'll assume and use NOVAS IDs that match NAIF.
 *
 * @since 1.2
 *
 * @sa solsys-calceph.c
 * @sa solsys-cspice.c
 */

#include <errno.h>

#define __NOVAS_INTERNAL_API__      ///< Use definitions meant for internal use by SuperNOVAS only
#include "novas.h"

/**
 * Converts a NAIF ID to a NOVAS major planet ID. It account for the different IDs used for Sun,
 * Moon, SSB, EMB and the Pluto system. Otherwise NAIF planet barycenters are mapped to the
 * corresponding bodies. NAIF body centers <i>n</i>99 (e.g. 399 for Earth) are mapped to the
 * corresponding NOVAS planet number <i>n</i>. All other NAIF IDs will return -1, indicating no
 * match to a NOVAS planet ID.
 *
 *
 * @param id      The NAIF ID of the major planet of interest
 * @return        the NOVAS ID for the same object (which may or may not be different from the
 *                input), or -1 if the NAIF ID cannot be matched to a NOVAS major planet.
 *
 * @author Attila Kovacs
 * @since 1.2
 *
 * @sa novas_to_naif_planet(), novas_to_dexxx_planet()
 */
enum novas_planet naif_to_novas_planet(long id) {
  switch (id) {
    case NAIF_SUN:
      return NOVAS_SUN;
    case NAIF_MOON:
      return NOVAS_MOON;
    case NAIF_SSB:
      return NOVAS_SSB;
    case NAIF_EMB:
      return NOVAS_EMB;
    case NAIF_PLUTO_BARYCENTER:
      return NOVAS_PLUTO_BARYCENTER;
  }

  // Major planets
  if(id >= NOVAS_MERCURY && id <= NOVAS_PLUTO)
    return id;
  if(id > 100 && id < 1000) if(id % 100 == 99)
    return (id - 99)/100;

  return novas_error(-1, EINVAL, "naif_to_novas_planet", "Invalid NOVAS major planet no: %ld", id);
}

/**
 * Converts a NOVAS Solar-system body ID to a NAIF Solar-system body ID. NOVAS and NAIF use
 * slightly different IDs for major planets, the Moon, SSB, EMB, and the Pluto system. In NOVAS,
 * major planets are have IDs ranging from 1 through 9, but for NAIF 1--9 are the planetary
 * barycenters and the planet centers have numbers in the hundreds ending with 99 (e.g. the center
 * of Earth is NAIF 399; 3 is the NOVAS ID for Earth and the NAIF ID for the Earth-Moon Barycenter
 * [EMB]). The Sun and Moon also have distinct IDs in NAIF vs NOVAS.
 *
 *
 * @param id      The NOVAS ID of the major planet of interest
 * @return        the NAIF ID for the same object or planet center (which may or may not be
 *                different from the input)
 *
 * @author Attila Kovacs
 * @since 1.2
 *
 * @sa naif_to_novas_planet()
 */
long novas_to_naif_planet(enum novas_planet id) {
  if(id >= NOVAS_MERCURY && id <= NOVAS_PLUTO)
    return 100 * id + 99;     // 1-9: Major planets

  switch (id) {
    case NOVAS_SUN:
      return NAIF_SUN;
    case NOVAS_MOON:
      return NAIF_MOON;
    case NOVAS_SSB:
      return NAIF_SSB;
    case NOVAS_EMB:
      return NAIF_EMB;
    case NOVAS_PLUTO_BARYCENTER:
      return NAIF_PLUTO_BARYCENTER;
    default:
      return novas_error(-1, EINVAL, "novas_to_naif_planet", "Invalid NOVAS major planet no: %d", id);
  }
}

/**
 * Converts a NOVAS Solar-system body ID to a NAIF Solar-system body ID for DExxx ephemeris files.
 * The DExxx (e.g. DE440) ephemeris files use NAIF IDs, but for most planets contain barycentric
 * data only rather than that of the planet center. For Earth-based observations, it only really
 * makes a difference whether the 3 is used for the Earth-Moon Barycenter (EMB) or 399 for the
 * geocenter.
 *
 * @param id      The NOVAS ID of the major planet of interest
 * @return        the NAIF ID for the same object (which may or may not be different from the
 *                input), as appropriate for use in the DExxx ephemeris files.
 *
 * @author Attila Kovacs
 * @since 1.2
 *
 * @sa novas_to_naif_planet(), naif_to_novas_planet()
 */
long novas_to_dexxx_planet(enum novas_planet id) {

  switch (id) {
    case NOVAS_SUN:
      return NAIF_SUN;
    case NOVAS_EARTH:
      return NAIF_EARTH;
    case NOVAS_MOON:
      return NAIF_MOON;
    case NOVAS_SSB:
      return NAIF_SSB;
    case NOVAS_EMB:
      return NAIF_EMB;
    case NOVAS_PLUTO_BARYCENTER:
      return NAIF_PLUTO_BARYCENTER;
    default:
      return (id >= NOVAS_MERCURY && id <= NOVAS_PLUTO) ?
              (long) id :
              novas_error(-1, EINVAL, "novas_to_dexxx_planet", "Invalid NOVAS major planet no: %d", id);
  }
}


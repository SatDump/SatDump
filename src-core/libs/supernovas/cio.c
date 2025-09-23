/**
 * @file
 *
 * @date Created  on Mar 6, 2025
 * @author G. Kaplan and Attila Kovacs
 *
 *  Functions to handle the CIO location and basis.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>


/// \cond PRIVATE
#define __NOVAS_INTERNAL_API__    ///< Use definitions meant for internal use by SuperNOVAS only
#include "novas.h"

#define CIO_INTERP_POINTS   6     ///< Number of points to load from CIO interpolation table at once.

/// [bytes] Sizeof binary CIO locator file header
#define CIO_BIN_HEADER_SIZE   (3*sizeof(double) + sizeof(long))

/// \endcond


///< Opened CIO locator data file, or NULL.
static FILE *cio_file;

/**
 * Computes the true right ascension of the celestial intermediate origin (CIO) vs the equinox of
 * date on the true equator of date for a given TT Julian date. This is simply the negated return
 * value ofira_equinox() for the true equator of date.
 *
 * REFERENCES:
 * <ol>
 * <li>Kaplan, G. (2005), US Naval Observatory Circular 179.</li>
 * </ol>
 *
 * @param jd_tt       [day] Terrestrial Time (TT) based Julian date
 * @param accuracy    NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] ra_cio [h] Right ascension of the CIO, with respect to the true equinox of date, in
 *                    hours (+ or -), or NAN when returning with an error code.
 * @return            0 if successful, -1 if the output pointer argument is NULL,
 *                    1 if 'accuracy' is invalid, 10--20: 10 + error code from cio_location(), or
 *                    else 20 + error from cio_basis()
 *
 * @sa ira_equinox()
 */
short cio_ra(double jd_tt, enum novas_accuracy accuracy, double *restrict ra_cio) {
  static const char *fn = "cio_ra";

  if(!ra_cio)
    return novas_error(-1, EINVAL, fn, "NULL output array");

  // Default return value.
  *ra_cio = NAN;

  // Check for valid value of 'accuracy'.
  if(accuracy != NOVAS_FULL_ACCURACY && accuracy != NOVAS_REDUCED_ACCURACY)
    return novas_error(1, EINVAL, fn, "invalid accuracy: %d", accuracy);

  // For these calculations we can assume TDB = TT (< 2 ms difference)
  *ra_cio = -ira_equinox(jd_tt, NOVAS_TRUE_EQUINOX, accuracy);
  return 0;
}

/**
 * @deprecated    SuperNOVAS no longer uses a NOVAS-type CIO locator file. However, if users want
 *                to access such files  directly, this function remains accessible to provide API
 *                compatibility with previous versions.
 *
 * Sets the CIO interpolaton data file to use to interpolate CIO locations vs the GCRS. You can
 * specify either the original `CIO_RA.TXT` file included in the distribution (preferred since
 * v1.1), or else a platform-specific binary data file compiled from it via the
 * <code>cio_file</code> utility (the old way).
 *
 * @param filename    Path (preferably absolute path) `CIO_RA.TXT` or else to the binary
 *                    `cio_ra.bin` data, or NULL to disable using a CIO locator file altogether.
 * @return            0 if successful, or else -1 if the specified file does not exists or we have
 *                    no permission to read it.
 *
 * @sa cio_location()
 *
 * @since 1.0
 * @author Attila Kovacs
 */
int set_cio_locator_file(const char *restrict filename) {
  FILE *old = cio_file;

  // Open new file first to ensure it has a distinct pointer from the old one...
  cio_file = filename ? fopen(filename, "r") : NULL;

  // Close the old file.
  if(old)
    fclose(old);

  if(!filename)
    return 0;

  return cio_file ? 0 : novas_error(-1, errno, "set_cio_locator_file", "%s: %s", filename, strerror(errno));
}

/**
 * @deprecated This function is no longer used internally in the library. Given that the CIO
 *             is defined on the dynamical equator of date, it is not normally meaningful to
 *             provide an R.A. coordinate for it in GCRS, in general. Instead, you might use
 *             cio_ra() to get the same value w.r.t. the equinox of date (on the same CIRS/TOD
 *             dynamical equator, or else `ira_equinox()` to return the negated value.
 *
 * Returns the location of the celestial intermediate origin (CIO) for a given Julian date, as a
 * right ascension with respect to either the GCRS (geocentric ICRS) origin or the true equinox of
 * date. The CIO is always located on the true equator (= intermediate equator) of date.
 *
 * The user may specify an interpolation file to use via set_cio_locator_file() prior to calling
 * this function. In that case the call will return CIO location relative to GCRS. In the absence
 * of the table, it will calculate the CIO location relative to the true equinox. In either case,
 * the type of the location is returned alongside the corresponding CIO location value.
 *
 * NOTES:
 * <ol>
 * <li>
 *   Unlike the NOVAS C version of this function, this version will always return a CIO
 *   location as long as the pointer arguments are not NULL. The returned values will be
 *   interpolated from the locator file if possible, otherwise it falls back to calculating an
 *   equinox-based location per default.
 * </li>
 * <li>
 *  This function caches the results of the last calculation in case it may be re-used at no extra
 *  computational cost for the next call.
 * </li>
 * <li>
 *  Note, that when a CIO location file is used, the bundled CIO locator data was prepared with
 *  the original IAU2000A nutation model, not with the newer R06 (a.k.a. IAU2006) model, resulting
 *  in an error up to the few tens of micro-arcseconds level for dates between 1900 and 2100, and
 *  larger errors further away from the current epoch.
 * </li>
 * </ol>
 *
 * @param jd_tdb           [day] Barycentric Dynamic Time (TDB) based Julian date
 * @param accuracy         NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] ra_cio      [h] Right ascension of the CIO, in hours, or NAN if returning with an
 *                         error.
 * @param[out] loc_type    Pointer in which to return the reference system in which right
 *                         ascension is given, which is either CIO_VS_GCRS (1) if the location was
 *                         obtained via interpolation of the available data file, or else
 *                         CIO_VS_EQUINOX (2) if it was calculated locally. It is set to -1 if
 *                         returning with an error.
 *
 * @return            0 if successful, -1 if one of the pointer arguments is NULL or the
 *                    accuracy is invalid.
 *
 * @sa cio_ra(), ira_equinox()
 */
short cio_location(double jd_tdb, enum novas_accuracy accuracy, double *restrict ra_cio, short *restrict loc_type) {
  static const char *fn = "cio_location";

  static THREAD_LOCAL enum novas_accuracy acc_last = -1;
  static THREAD_LOCAL short ref_sys_last = -1;
  static THREAD_LOCAL double t_last = NAN, ra_last = NAN;
  static THREAD_LOCAL ra_of_cio cio[CIO_INTERP_POINTS];

  const enum novas_debug_mode saved_debug_state = novas_get_debug_mode();

  // Default return values...
  if(ra_cio)
    *ra_cio = NAN;
  if(loc_type)
    *loc_type = -1;

  if(!ra_cio || !loc_type)
    return novas_error(-1, EINVAL, fn, "NULL output pointer: ra_cio=%p, loc_type=%p", ra_cio, loc_type);

  if(accuracy != NOVAS_FULL_ACCURACY && accuracy != NOVAS_REDUCED_ACCURACY)
    return novas_error(-1, EINVAL, fn, "invalid accuracy: %d", accuracy);

  // Check if previously computed RA value can be used.
  if(novas_time_equals(jd_tdb, t_last) && (accuracy == acc_last)) {
    *ra_cio = ra_last;
    *loc_type = ref_sys_last;
    return 0;
  }

  // We can let slide errors from cio_array since they don't bother us.
  if(saved_debug_state == NOVAS_DEBUG_ON)
    novas_debug(NOVAS_DEBUG_OFF);

  if(cio_array(jd_tdb, CIO_INTERP_POINTS, cio) == 0) {
    int j;

    // Restore the user-selected debug mode.
    novas_debug(saved_debug_state);

    // Perform Lagrangian interpolation for the RA at 'tdb_jd'.
    *ra_cio = 0.0;

    for(j = 0; j < CIO_INTERP_POINTS; j++) {
      double p = 1.0;
      int i;
      for(i = 0; i < CIO_INTERP_POINTS; i++)
        if(i != j)
          p *= (jd_tdb - cio[i].jd_tdb) / (cio[j].jd_tdb - cio[i].jd_tdb);
      *ra_cio += p * cio[j].ra_cio;
    }

    // change units from arcsec to hour-angle (express as arcsec [*], then cast as hour-angle [/])
    *ra_cio *= ARCSEC / HOURANGLE;
    *loc_type = CIO_VS_GCRS;
  }
  else {
    // Restore the user-selected debug mode.
    novas_debug(saved_debug_state);

    // Calculate the equation of origins.
    *ra_cio = -ira_equinox(jd_tdb, NOVAS_TRUE_EQUINOX, accuracy);
    *loc_type = CIO_VS_EQUINOX;
  }

  t_last = jd_tdb;
  acc_last = accuracy;
  ra_last = *ra_cio;
  ref_sys_last = *loc_type;

  return 0;
}

/**
 * @deprecated This function is no longer used internally in the library, and users are
 *             recommended against using it themselves, since SuperNOVAS provides better ways to
 *             convert between GCRS and CIRS using frames or via gcrs_to_cirs() / cirs_to_gcrs()
 *             functions.
 *
 * Computes the CIRS basis vectors, with respect to the GCRS (geocentric ICRS), of the
 * celestial intermediate system defined by the celestial intermediate pole (CIP) (in the z
 * direction) and the celestial intermediate origin (CIO) (in the x direction).
 *
 * This function effectively constructs the CIRS to GCRS transformation matrix C in eq. (3) of the
 * reference.
 *
 * NOTES:
 * <ol>
 * <li>This function caches the results of the last calculation in case it may be re-used at
 * no extra computational cost for the next call.</li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 * <li>Kaplan, G. (2005), US Naval Observatory Circular 179.</li>
 * </ol>
 *
 * @param jd_tdb      [day] Barycentric Dynamic Time (TDB) based Julian date
 * @param ra_cio      [h] Right ascension of the CIO at epoch (hours).
 * @param loc_type    CIO_VS_GCRS (1) if the cio location is relative to the GCRS or else
 *                    CIO_VS_EQUINOX (2) if relative to the true equinox of date.
 * @param accuracy    NOVAS_FULL_ACCURACY (0) or NOVAS_REDUCED_ACCURACY (1)
 * @param[out] x      Unit 3-vector toward the CIO, equatorial rectangular coordinates,
 *                    referred to the GCRS.
 * @param[out] y      Unit 3-vector toward the y-direction, equatorial rectangular
 *                    coordinates, referred to the GCRS.
 * @param[out] z      Unit 3-vector toward north celestial pole (CIP), equatorial
 *                    rectangular coordinates, referred to the GCRS.
 * @return            0 if successful, or -1 if any of the output vector arguments are NULL
 *                    or if the accuracy is invalid, or else 1 if 'ref-sys' is invalid.
 *
 * @sa gcrs_to_cirs(), cirs_to_gcrs(), novas_make_transform()
 */
short cio_basis(double jd_tdb, double ra_cio, enum novas_cio_location_type loc_type, enum novas_accuracy accuracy,
        double *restrict x, double *restrict y, double *restrict z) {
  static const char *fn = "cio_basis";
  static THREAD_LOCAL enum novas_accuracy acc_last = -1;
  static THREAD_LOCAL double t_last = NAN;
  static THREAD_LOCAL double xx[3], yy[3], zz[3];

  if(!x || !y || !z)
    return novas_error(-1, EINVAL, fn, "NULL output 3-vector: x=%p, y=%p, z=%p", x, y, z);

  if(accuracy != NOVAS_FULL_ACCURACY && accuracy != NOVAS_REDUCED_ACCURACY)
    return novas_error(-1, EINVAL, fn, "invalid accuracy: %d", accuracy);

  // Compute unit vector z toward celestial pole.
  if(!zz[2] || !novas_time_equals(jd_tdb, t_last) || (accuracy != acc_last)) {
    const double z0[3] = { 0.0, 0.0, 1.0 };
    tod_to_gcrs(jd_tdb, accuracy, z0, zz);
    t_last = jd_tdb;
    acc_last = accuracy;

    // Now compute unit vectors x and y.  Method used depends on the
    // reference system in which right ascension of the CIO is given.
    ra_cio *= HOURANGLE;

    switch(loc_type) {
      case CIO_VS_GCRS: {

        // Compute vector x toward CIO in GCRS.
        const double sinra = sin(ra_cio);
        const double cosra = cos(ra_cio);
        double l;

        xx[0] = zz[2] * cosra;
        xx[1] = zz[2] * sinra;
        xx[2] = -zz[0] * cosra - zz[1] * sinra;

        // Normalize vector x.
        l = novas_vlen(xx);
        xx[0] /= l;
        xx[1] /= l;
        xx[2] /= l;

        break;
      }

      case CIO_VS_EQUINOX: {
        // Construct unit vector toward CIO in equator-and-equinox-of-date
        // system.
        xx[0] = cos(ra_cio);
        xx[1] = sin(ra_cio);
        xx[2] = 0.0;

        // Rotate the vector into the GCRS to form unit vector x.
        tod_to_gcrs(jd_tdb, accuracy, xx, xx);
        break;
      }

      default:
        // Invalid value of 'ref_sys'.
        memset(x, 0, XYZ_VECTOR_SIZE);
        memset(y, 0, XYZ_VECTOR_SIZE);
        memset(z, 0, XYZ_VECTOR_SIZE);

        return novas_error(1, EINVAL, fn, "invalid input CIO location type: %d", loc_type);
    }

    // Compute unit vector y orthogonal to x and z (y = z cross x).
    yy[0] = zz[1] * xx[2] - zz[2] * xx[1];
    yy[1] = zz[2] * xx[0] - zz[0] * xx[2];
    yy[2] = zz[0] * xx[1] - zz[1] * xx[0];
  }

  memcpy(x, xx, sizeof(xx));
  memcpy(y, yy, sizeof(yy));
  memcpy(z, zz, sizeof(zz));

  return 0;
}

/**
 * @deprecated    This function is no longer used within SuperNOVAS. It is still provided,
 *                however, in order to retain 100% API compatibility with NOVAS C.
 *
 * Given an input TDB Julian date and the number of data points desired, this function returns
 * a set of Julian dates and corresponding values of the GCRS right ascension of the celestial
 * intermediate origin (CIO).  The range of dates is centered (at least approximately) on the
 * requested date.  The function obtains the data from an external data file.
 *
 * This function assumes that a CIO locator file (`CIO_RA.TXT` or `cio_ra.bin`) exists in the
 * default location (configured at build time), or else was specified via `set_cio_locator_file()`
 * prior to calling this function.
 *
 * NOTES:
 * <ol>
 * <li>
 *   This function has been completely re-written by A. Kovacs to provide much more efficient
 *   caching and I/O.
 * </li>
 * <li>
 *  The CIO locator file that is bundled was prepared with the original IAU2000A nutation model,
 *  not with the newer R06 (a.k.a. IAU2006) nutation model, resulting in an error up to the few
 *  tens of micro-arcseconds level for dates between 1900 and 2100, and larger errors further away
 *  from the current epoch.
 * </li>
 * </ol>
 *
 * @param jd_tdb    [day] Barycentric Dynamic Time (TDB) based Julian date
 * @param n_pts     Number of Julian dates and right ascension values requested (not less than 2
 *                  or more than NOVAS_CIO_CACHE_SIZE).
 * @param[out] cio  A time series (array) of the right ascension of the Celestial Intermediate
 *                  Origin (CIO) with respect to the GCRS.
 * @return          0 if successful, -1 if the output array is NULL or there
 *                  was an I/O error accessing the CIO location data file. Or else 1 if no
 *                  locator data file is available, 2 if 'jd_tdb' not in the range of the CIO
 *                  file, 3 if 'n_pts' out of range, or 6 if 'jd_tdb' is too close to either end
 *                  of the CIO file do we are unable to put 'n_pts' data points into the output
 *
 * @sa set_cio_locator_file(), cio_location()
 */
short cio_array(double jd_tdb, long n_pts, ra_of_cio *restrict cio) {
  static const char *fn = "cio_array";

  // Packed struct in case long is not the same width a double
  struct cio_file_header {
    double jd_start;
    double jd_end;
    double jd_interval;
    long n_recs;
  };

  static const FILE *last_file;
  static struct cio_file_header lookup;
  static ra_of_cio cache[NOVAS_CIO_CACHE_SIZE];
  static long index_cache, cache_count;
  static int is_ascii;
  static int header_size, lrec;

  long index_rec;

  if(cio == NULL)
    return novas_error(-1, EINVAL, fn, "NULL output array");

  if(n_pts < 2 || n_pts > NOVAS_CIO_CACHE_SIZE)
    return novas_error(3, ERANGE, fn, "n_pts=%ld is out of bounds [2:%d]", n_pts, NOVAS_CIO_CACHE_SIZE);

  if(cio_file == NULL)
    set_cio_locator_file(DEFAULT_CIO_LOCATOR_FILE);  // Try default locator file.

  if(cio_file == NULL)
    return novas_error(1, ENOENT, fn, "No default CIO locator file");

  // Check if it's a new file
  if(last_file != cio_file) {
    char line[80] = {0};
    int version, tokens;

    last_file = NULL;
    cache_count = 0;

    if(fgets(line, sizeof(line) - 1, cio_file) == NULL)
      return novas_error(1, errno, fn, "empty CIO locator data: %s", strerror(errno));

    tokens = sscanf(line, "CIO RA P%d @ %lfd", &version, &lookup.jd_interval);

    if(tokens == 2) {
      is_ascii = 1;
      header_size = strlen(line);

      if(fgets(line, sizeof(line) - 1, cio_file) == NULL)
        return novas_error(1, errno, fn, "missing ASCII CIO locator data: %s", strerror(errno));

      lrec = strlen(line);

      if(sscanf(line, "%lf", &lookup.jd_start) < 1)
        return novas_error(-1, errno, fn, "incomplete or corrupted ASCII CIO locator record: %s", strerror(errno));

      fseek(cio_file, 0, SEEK_END);

      lookup.n_recs = (ftell(cio_file) - header_size) / lrec;
      lookup.jd_end = lookup.jd_start + lookup.n_recs * lookup.jd_interval;
    }
    else if(tokens) {
      return novas_error(-1, errno, fn, "incomplete or corrupted ASCII CIO locator data header: %s", strerror(errno));
    }
    else {
      is_ascii = 0;
      header_size = CIO_BIN_HEADER_SIZE;
      lrec = sizeof(ra_of_cio);

      fseek(cio_file, 0, SEEK_SET);

      // Read the file header
      if(fread(&lookup, header_size, 1, cio_file) != 1)
        return novas_error(-1, errno, fn, "incomplete or corrupted binary CIO locator data header: %s", strerror(errno));
    }

    last_file = cio_file;
  }

  // Check the input data against limits.
  if((jd_tdb < lookup.jd_start) || (jd_tdb > lookup.jd_end))
    return novas_error(2, EOF, fn, "requested time (JD=%.1f) outside of CIO locator data range (%.1f:%.1f)", jd_tdb, lookup.jd_start,
            lookup.jd_end);

  // Calculate the record index from which data is requested.
  index_rec = (long) ((jd_tdb - lookup.jd_start) / lookup.jd_interval) - (n_pts >> 1);
  if(index_rec < 0)
    return novas_error(6, EOF, fn, "not enough CIO location data points available at the requested time (JD=%.1f)", jd_tdb);

  // Check if the range of data needed is outside the cached range.
  if((index_rec < index_cache) || (index_rec + n_pts > index_cache + cache_count)) {
    // Load cache centered on requested range.
    const long N = lookup.n_recs - index_rec > NOVAS_CIO_CACHE_SIZE ? NOVAS_CIO_CACHE_SIZE : lookup.n_recs - index_rec;

    cache_count = 0;
    index_cache = index_rec - (NOVAS_CIO_CACHE_SIZE >> 1);
    if(index_cache < 0)
      index_cache = 0;

    // Read in cache from the requested position
    fseek(cio_file, header_size + index_cache * lrec, SEEK_SET);

    if(is_ascii) {
      for(cache_count = 0; cache_count < N; cache_count++)
        if(fscanf(cio_file, "%lf %lf\n", &cache[cache_count].jd_tdb, &cache[cache_count].ra_cio) != 2)
          return novas_error(-1, errno, fn, "corrupted ASCII CIO locator data: %s", strerror(errno));
    }
    else {
      if(fread(cache, sizeof(ra_of_cio), N, cio_file) != (size_t) N)
        return novas_error(-1, errno, fn, "corrupted binary CIO locator data: %s", strerror(errno));
      cache_count = N;
    }
  }

  if((index_rec - index_cache) + n_pts > cache_count)
    return novas_error(6, EOF, fn, "not enough CIO location data points available at the requested time (JD=%.1f)", jd_tdb);

  // Copy the requested number of points in to the destination;
  memcpy(cio, &cache[index_rec - index_cache], n_pts * sizeof(ra_of_cio));
  return 0;
}

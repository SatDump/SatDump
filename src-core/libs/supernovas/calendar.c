/**
 * @file
 *
 * @date Created  on Mar 6, 2025
 * @author Attila Kovacs and G. Kaplan
 *
 *  Various functions to convert between calendar dates and Julian days.
 */

#include <errno.h>

/// \cond PRIVATE
#define __NOVAS_INTERNAL_API__    ///< Use definitions meant for internal use by SuperNOVAS only
#include "novas.h"
/// \endcond


/**
 * Returns the Julian day for a given calendar date. Input time value can be based on any
 * astronomical time scale (UTC, UT1, TT, etc.) - output Julian date will have the same basis.
 *
 * The input date is the conventional calendar date, affected by the Gregorian calendar
 * reform of 1582. Thus, the input date is for the Gregorian calendar for dates starting
 * 15 October 1582, and for the Julian (Roman) calendar (introduced in 45 B.C.) for
 * dates prior to that.
 *
 * NOTES:
 * <ol>
 * <li>B.C. dates are indicated with years &lt;=0 according to the astronomical
 * and ISO 8601 convention, i.e., X B.C. as (1-X), so 45 B.C. as -44.</li>
 *
 * <li>Added argument range checking in v1.3.0, returning NAN if the month or day are out of
 * the normal range (for a leap year).</li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 *  <li>Fliegel, H. &amp; Van Flandern, T.  Comm. of the ACM, Vol. 11, No. 10, October 1968, p.
 *  657.</li>
 * </ol>
 *
 * @param calendar  The type of calendar to use: NOVAS_ASTRONOMICAL_CALENDAR,
 *                  NOVAS_GREGORIAN_CALENDAR, or NOVAS_ROMAN_CALENDAR.
 * @param year      [yr] Calendar year. B.C. years can be simply represented as
 *                  negative years, e.g. 1 B.C. as -1.
 * @param month     [month] Calendar month [1:12]
 * @param day       [day] Day of month [1:31]
 * @param hour      [hr] Hour of day [0:24]
 * @return          [day] the fractional Julian day for the input calendar date, ot NAN if
 *                  the calendar is invalid or the month or day components are out of range.
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_jd_to_date(), novas_get_time()
 */
double novas_jd_from_date(enum novas_calendar_type calendar, int year, int month, int day, double hour) {
  static const char *fn = "novas_jd_from_date";
  static const char md[13] = { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  long jd, m14 = month - 14L;
  double fjd;

  if(calendar < NOVAS_ROMAN_CALENDAR || calendar > NOVAS_GREGORIAN_CALENDAR)
    return novas_error(-1, EINVAL, fn, "invalid calendar type: %d\n", calendar);

  if(month < 1 || month > 12) {
    novas_set_errno(EINVAL, fn, "invalid month: %d, expected 1-12", month);
    return NAN;
  }

  if(day < 1 || day > md[month]) {
    novas_set_errno(EINVAL, fn, "invalid day-of-month: %d, expected 1-%d", day, md[month]);
    return NAN;
  }

  jd = day - 32123L + 1461L * (year + 4800L + m14 / 12L) / 4L + 367L * (month - 2L - m14 / 12L * 12L) / 12L;
  fjd = (hour - 12.0) / DAY_HOURS;

  if(calendar == NOVAS_ASTRONOMICAL_CALENDAR)
    calendar = (jd + fjd >= NOVAS_JD_START_GREGORIAN) ? NOVAS_GREGORIAN_CALENDAR : NOVAS_ROMAN_CALENDAR;

  if(calendar == NOVAS_GREGORIAN_CALENDAR)
    jd -= 3L * ((year + 4900L + m14 / 12L) / 100L) / 4L - 48L;  // Gregorian calendar reform
  else
    jd += 10L;                                                  // Julian (Roman) calendar

  return jd + fjd;
}

/**
 * This function will compute a broken down date on the specified calendar for given the
 * Julian day input. Input Julian day can be based on any astronomical time scale (UTC, UT1,
 * TT, etc.) - output time value will have same basis.
 *
 * NOTES:
 * <ol>
 *  <li>B.C. dates are indicated with years &lt;=0 according to the astronomical
 * and ISO 8601 convention, i.e., X B.C. as (1-X), so 45 B.C. as -44.</li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 *  <li>Fliegel, H. &amp; Van Flandern, T.  Comm. of the ACM, Vol. 11, No. 10, October 1968,
 *  p. 657.</li>
 * </ol>
 *
 * @param tjd          [day] Julian day.
 * @param calendar     The type of calendar to use: NOVAS_ASTRONOMICAL_CALENDAR,
 *                     NOVAS_GREGORIAN_CALENDAR, or NOVAS_ROMAN_CALENDAR.
 * @param[out] year    [yr] Calendar year. B.C. years are represented as negative values,
 *                     e.g. -1 corresponds to 1 B.C. It may be NULL if not required.
 * @param[out] month   [month] Calendar month [1:12]. It may be NULL if not
 *                     required.
 * @param[out] day     [day] Day of the month [1:31]. It may be NULL if not required.
 * @param[out] hour    [h] Hour of day [0:24]. It may be NULL if not required.
 *
 * @return             0 if successful, or else -1 if the calendar is invalid (errno will
 *                     be set to EINVAL).
 *
 * @since 1.3
 * @author Attila Kovacs
 *
 * @sa novas_jd_from_date(), novas_set_time()
 */
int novas_jd_to_date(double tjd, enum novas_calendar_type calendar, int *restrict year, int *restrict month,
        int *restrict day, double *restrict hour) {
  long jd, k, m, n;
  int y, mo, d;
  double djd, h;

  if(calendar < NOVAS_ROMAN_CALENDAR || calendar > NOVAS_GREGORIAN_CALENDAR)
    return novas_error(-1, EINVAL, "novas_jd_to_date", "invalid calendar type: %d\n", calendar);

  // Default return values
  if(year)
    *year = 0;
  if(month)
    *month = 0;
  if(day)
    *day = 0;
  if(hour)
    *hour = NAN;

  djd = tjd + 0.5;
  jd = (long) floor(djd);

  h = remainder(djd, 1.0) * DAY_HOURS;
  if(h < 0.0)
    h += 24.0;

  k = jd + 68569L;
  n = 4L * k / 146097L;

  if(calendar == NOVAS_ASTRONOMICAL_CALENDAR)
    calendar = (tjd >= NOVAS_JD_START_GREGORIAN) ? NOVAS_GREGORIAN_CALENDAR : NOVAS_ROMAN_CALENDAR;

  if(calendar == NOVAS_GREGORIAN_CALENDAR)
    k -= (146097L * n + 3L) / 4L;
  else
    k -= (146100L * n + 3L) / 4L;

  m = 4000L * (k + 1L) / 1461001L;

  k += 31L - 1461L * m / 4L;

  if(calendar == NOVAS_ROMAN_CALENDAR)
    k += 38L;

  mo = (int) (80L * k / 2447L);
  d = (int) (k - 2447L * (long) mo / 80L);
  k = mo / 11L;

  mo = (int) ((long) mo + 2L - 12L * k);
  y = (int) (100L * (n - 49L) + m + k);

  if(year)
    *year = y;
  if(month)
    *month = mo;
  if(day)
    *day = d;
  if(hour)
    *hour = h;

  return 0;
}

/**
 * Returns the Julian day for a given astronomical calendar date. Input time value can be based
 * on any UT-like time scale (UTC, UT1, TT, etc.) - output Julian day will have the same basis.
 *
 * NOTES:
 * <ol>
 * <li>The Gregorian calendar was introduced on 1582 October 15 only. Prior to that, astronomical
 * dates are Julian/Roman dates, so the day before the reform was 1582 October 4. You can also
 * use `novas_jd_from_date()` to convert dates with more flexibility.</li>
 *
 * <li>B.C. dates are indicated with years &lt;=0 according to the astronomical
 * and ISO 8601 convention, i.e., X B.C. as (1-X), so 45 B.C. as -44.</li>
 *
 * <li>Added argument range checking in v1.3.0, returning NAN if the month or day are out of
 * the normal range (for a leap year).</li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 *  <li>Fliegel, H. &amp; Van Flandern, T.  Comm. of the ACM, Vol. 11, No. 10, October 1968, p.
 *  657.</li>
 * </ol>
 *
 * @param year    [yr] Astronomical calendar year. B.C. years can be simply represented as
 *                &lt;=0, e.g. 1 B.C. as 0, and _X_ B.C. as (1 - _X_).
 * @param month   [month] Astronomical calendar month [1:12]
 * @param day     [day] Astronomical day of month [1:31]
 * @param hour    [hr] Hour of day [0:24]
 * @return        [day] the fractional Julian date for the input calendar date, ot NAN if
 *                month or day components are out of range.
 *
 * @sa novas_jd_from_date()
 * @sa get_utc_to_tt(), get_ut1_to_tt(), tt2tdb()
 */
double julian_date(short year, short month, short day, double hour) {
  double jd = novas_jd_from_date(NOVAS_ASTRONOMICAL_CALENDAR, year, month, day, hour);
  if(isnan(jd))
    return novas_trace_nan("julian_date");
  return jd;
}

/**
 * This function will compute a broken down date on the astronomical calendar for
 * given the Julian day input. Input Julian day can be based on any UT-like time scale
 * (UTC, UT1, TT, etc.) - output time value will have same basis.
 *
 * NOTES:
 * <ol>
 * <li>The Gregorian calendar was introduced on 15 October 1582 only (corresponding to 5
 * October of the previously used Julian calendar). Prior to it this function returns
 * Julian/Roman calendar dates, e.g. the day before the reform is 1582 October 4. You can
 * use `novas_jd_to_date()` instead to convert JD days to dates in specific
 * calendars.</li>
 *
 * <li>B.C. dates are indicated with years &lt;=0 according to the astronomical
 * and ISO 8601 convention, i.e., X B.C. as (1-X), so 45 B.C. as -44.</li>
 * </ol>
 *
 * REFERENCES:
 * <ol>
 *
 *  <li>Fliegel, H. &amp; Van Flandern, T.  Comm. of the ACM, Vol. 11, No. 10, October 1968,
 *  p. 657.</li>15 October 1582
 * </ol>
 *
 * @param tjd          [day] Julian date
 * @param[out] year    [yr] Astronomical calendar year. It may be NULL if not required.
 *                     B.C. years are represented as &lt;=0, i.e. 1 B.C. as 0 and _X_ B.C.
 *                     as (1 - _X_)
 * @param[out] month   [month] Astronomical calendar month [1:12]. It may be NULL if not
 *                     required.
 * @param[out] day     [day] Day of the month [1:31]. It may be NULL if not required.
 * @param[out] hour    [h] Hour of day [0:24]. It may be NULL if not required.
 *
 * @return              0
 *
 * @sa novas_jd_to_date()
 * @sa novas_get_time()
 */
int cal_date(double tjd, short *restrict year, short *restrict month, short *restrict day, double *restrict hour) {
  int y, m, d;

  novas_jd_to_date(tjd, NOVAS_ASTRONOMICAL_CALENDAR, &y, &m, &d, hour);

  if(year)
    *year = (short) y;
  if(month)
    *month = (short) m;
  if(day)
    *day = (short) d;

  return 0;
}

/**
 * Returns the one-based ISO 8601 day-of-week index of a given Julian Date. The ISO 8601
 * week begins on Monday, thus index 1 corresponds to Monday, while index 7 is a Sunday.
 *
 * @param tjd   [day] Julian Date in the timescale of choice. (e.g. UTC-based if you want
 *              a UTC-based return value).
 * @return      [1:7] The day-of-week index in the same timescale as the input date.
 *              1:Monday ... 7:Sunday.
 *
 * @since 1.4
 * @author Attila Kovacs
 *
 * @sa novas_day_of_year(), novas_jd_to_date()
 */
int novas_day_of_week(double tjd) {
  int rem = (int) ((long) floor(tjd - NOVAS_JD_J2000 - 1.5) % 7L);
  if(rem < 0)
    rem += 7;
  return 1 + rem;
}

/**
 * Returns the one-based day index in the calendar year for a given Julian Date.
 *
 * @param tjd       [day] Julian Date in the timescale of choice. (e.g. UTC-based if you want
 *                  a UTC-based return value).
 * @param calendar  The type of calendar to use: NOVAS_ASTRONOMICAL_CALENDAR,
 *                  NOVAS_GREGORIAN_CALENDAR, or NOVAS_ROMAN_CALENDAR.
 * @param[out] year [yr] Optional pointer to which to return the calendar year. It may be
 *                  NULL if not required.
 * @return          [1:366] The day-of-year index in the same timescale as the input date.
 *
 * @since 1.4
 * @author Attila Kovacs
 *
 * @sa novas_day_of_week(), novas_jd_to_date()
 */
int novas_day_of_year(double tjd, enum novas_calendar_type calendar, int *restrict year) {
  static const int mstart[] = { 0, 31, 59, 9, 120, 151, 181, 212, 243, 273, 304, 334 }; // non-leap year month offsets

  int y = 0, m = 0, d = -1;
  int yday;

  prop_error("novas_day_of_year", novas_jd_to_date(tjd, calendar, &y, &m, &d, NULL), 0);
  yday = mstart[m - 1] + d;

  // Adjust for leap years
  if(m > 2 && (y % 4) == 0) {
    if((y % 100) != 0)
      yday++;   // regular leap years (not 100s)...
    else if((y % 400) == 0)
      yday++;   // every 400 is always a leap too...
    else if(calendar == NOVAS_ROMAN_CALENDAR)
      yday++;   // in the Roman/Julian calendar every 100 years is a leap...
    else if(calendar == NOVAS_ASTRONOMICAL_CALENDAR && tjd < NOVAS_JD_START_GREGORIAN)
      yday++;   // in the astronomical calendar every 100 is a leap before the calendar reform of 1582.
  }

  if(year)
    *year = y;

  return yday;
}

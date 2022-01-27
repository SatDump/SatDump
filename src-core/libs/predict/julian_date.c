#include "predict.h"

#include <stdio.h>
#include <time.h>
#include <math.h>

#include "defs.h"

/**
 * Create time_t in UTC from struct tm.
 *
 * \param timeinfo_utc Broken down time, assumed to be in UTC
 * \return Time in UTC
 **/
time_t mktime_utc(const struct tm *timeinfo_utc)
{
	time_t curr_time = time(NULL);
	int timezone_diff = 0; //deviation of the current timezone from UTC in seconds

	//get UTC time, interpret resulting tm as a localtime
	struct tm timeinfo_gmt;
#ifdef _WIN32
	gmtime_s(&timeinfo_gmt, &curr_time);
#else
	gmtime_r(&curr_time, &timeinfo_gmt);
#endif
	time_t time_gmt = mktime(&timeinfo_gmt);

	//get localtime, interpret resulting tm as localtime
	struct tm timeinfo_local;
#ifdef _WIN32
	localtime_s(&timeinfo_local, &curr_time);
#else
	localtime_r(&curr_time, &timeinfo_local);
#endif
	time_t time_local = mktime(&timeinfo_local);

	//find the time difference between the two interpretations
	timezone_diff += difftime(time_local, time_gmt);

	//hack for preventing mktime from assuming localtime: add timezone difference to the input struct.
	struct tm ret_timeinfo;
	ret_timeinfo.tm_sec = timeinfo_utc->tm_sec + timezone_diff;
	ret_timeinfo.tm_min = timeinfo_utc->tm_min;
	ret_timeinfo.tm_hour = timeinfo_utc->tm_hour;
	ret_timeinfo.tm_mday = timeinfo_utc->tm_mday;
	ret_timeinfo.tm_mon = timeinfo_utc->tm_mon;
	ret_timeinfo.tm_year = timeinfo_utc->tm_year;
	ret_timeinfo.tm_isdst = timeinfo_utc->tm_isdst;
	return mktime(&ret_timeinfo);
}

/**
 * Helper function for getting the Julian day start date (1979-12-31 00:00 UTC) as time_t.
 *
 * \return Internally defined Julian start date (fixed)
 **/
time_t get_julian_start_day()
{
	struct tm start_time;
	start_time.tm_sec = 0;
	start_time.tm_min = 0;
	start_time.tm_hour = 0;
	start_time.tm_mday = 31;
	start_time.tm_mon = 11;
	start_time.tm_year = 1979 - 1900;
	start_time.tm_isdst = 0;
	return mktime_utc(&start_time);
}

predict_julian_date_t predict_to_julian(time_t input_time)
{
	//get number of seconds since 1979-12-31 00:00:00 UTC, convert to days
	double seconds = difftime(input_time, get_julian_start_day());
	return seconds / SECONDS_PER_DAY;
}

predict_julian_date_t predict_to_julian_double(double time) 
{
 	return predict_to_julian(time) + (fmod(time, 1.0) / SECONDS_PER_DAY);
}

time_t predict_from_julian(predict_julian_date_t date)
{
	double seconds_since = date * SECONDS_PER_DAY;
	time_t ret_time = get_julian_start_day();

	//add number of seconds since julian start day to the julian start day, get current time_t
	struct tm timeinfo;
#ifdef _WIN32
	gmtime_s(&timeinfo, &ret_time);
#else
	gmtime_r(&ret_time, &timeinfo);
#endif
	timeinfo.tm_sec += seconds_since;
	ret_time = mktime_utc(&timeinfo);
	return ret_time;
}

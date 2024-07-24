#include "predict.h"

#include <stdio.h>
#include <time.h>
#include <math.h>

#include "defs.h"

#define JULIAN_START_OF_DAY 315446400

/**
 * Create time_t in UTC from struct tm.
 *
 * \param timeinfo_utc Broken down time, assumed to be in UTC
 * \return Time in UTC
 **/

predict_julian_date_t predict_to_julian(time_t input_time)
{
	//get number of seconds since 1979-12-31 00:00:00 UTC, convert to days
	double seconds = difftime(input_time, JULIAN_START_OF_DAY);
	return seconds / SECONDS_PER_DAY;
}

predict_julian_date_t predict_to_julian_double(double time) 
{
 	return predict_to_julian(time) + (fmod(time, 1.0) / SECONDS_PER_DAY);
}

time_t predict_from_julian(predict_julian_date_t date)
{
	return date * SECONDS_PER_DAY + JULIAN_START_OF_DAY;
}

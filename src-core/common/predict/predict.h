#ifndef _PREDICT_H_
#define _PREDICT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <stdbool.h>

#define PREDICT_VERSION_MAJOR 3
#define PREDICT_VERSION_MINOR 0
#define PREDICT_VERSION_PATCH 0
#define PREDICT_VERSION (PREDICT_VERSION_MAJOR * 100 * 100 + PREDICT_VERSION_MINOR * 100 + PREDICT_VERSION_PATCH)
#define PREDICT_VERSION_STRING "3.0.0"
  
/**
 * Get the major version number of the library
 *
 * \return Major version number
 */
int predict_version_major();

/**
 * Get the minor version number of the library
 *
 * \return Minor version number
 */
int predict_version_minor();

/**
 * Get the patch version number of the library
 *
 * \return Patch version number
 */
int predict_version_patch();

/**
 * Get the version number of the library
 *
 * \return 2 digit major, 2 digit minor, 2 digit patch decimal version number
 */
int predict_version();

/**
 * Get the version number string of the library
 *
 * \return Version number string ("major.minor.patch")
 */
char *predict_version_string();

/**
 * The representation of time used by libpredict: The number of days since 31Dec79 00:00:00 UTC. 
 **/
typedef double predict_julian_date_t;

/**
 * Convert time_t in UTC to Julian date in UTC.
 *
 * \param time Time in UTC
 * \return Julian day in UTC
 **/
predict_julian_date_t predict_to_julian(time_t time);

/**
 * Convert Julian date in UTC back to a time_t in UTC. 
 *
 * \param date Julian date in UTC
 * \return Time in UTC
 **/
time_t predict_from_julian(predict_julian_date_t date);

/**
 * Simplified perturbation models used in modeling the satellite orbits.
 **/
enum predict_ephemeris {
  EPHEMERIS_SGP4 = 0,
  EPHEMERIS_SDP4 = 1,
  EPHEMERIS_SGP8 = 2,
  EPHEMERIS_SDP8 = 3
};

/**
 * Container for processed TLE data from TLE strings.
 **/
typedef struct {
	///Satellite number (line 1, field 2)
	int satellite_number;
	///Element number (line 1, field 13)
	long element_number;
	///International designator (line 1, fields 4, 5, 6)
	char designator[10];
	///Epoch year (last two digits) (line 1, field 7)
	int epoch_year;
	///Epoch day (day of year and fractional portion of day, line 1, field 8)
	double epoch_day;
	///Inclination (line 2, field 3)
	double inclination;
	///Right Ascension of the Ascending Node [Degrees] (line 2, field 4)
	double right_ascension;
	///Eccentricity (line 2, field 5)
	double eccentricity;
	///Argument of Perigee [Degrees] (line 2, field 6)
	double argument_of_perigee;
	///Mean Anomaly [Degrees] (line 2, field 7)
	double mean_anomaly;
	///Mean Motion [Revs per day] (line 2, field 8)
	double mean_motion;
	///First Time Derivative of the Mean Motion divided by two (line 1, field 9)
	double derivative_mean_motion;
	///Second Time Derivative of Mean Motion divided by six (line 1, field 10)
	double second_derivative_mean_motion;
	///BSTAR drag term (decimal point assumed, line 1, field 11)
	double bstar_drag_term;
	///Number of revolutions around Earth at epoch (line 2, field 9)
	int revolutions_at_epoch;

	///Which perturbation model to use
	enum predict_ephemeris ephemeris;
	///Ephemeris data structure pointer
	void *ephemeris_data;
} predict_orbital_elements_t;

/**
 * Create predict_orbital_elements_t from TLE strings.
 *
 * \param tle_line_1 First line of NORAD two-line element set string
 * \param tle_line_2 Second line of NORAD two-line element set string
 * \return Processed TLE parameters
 * \copyright GPLv2+
 **/
predict_orbital_elements_t* predict_parse_tle(const char *tle_line_1, const char *tle_line_2);

/**
 * Free memory allocated in orbital elements structure.
 * \param orbital_elements Orbit to free
 **/
void predict_destroy_orbital_elements(predict_orbital_elements_t *orbital_elements);

/**
 * Predicted orbital values for satellite at a given time.
 **/
struct predict_position {
	///Timestamp for last call to orbit_predict
	predict_julian_date_t time;

	///Whether the orbit has decayed
	bool decayed;

	///ECI position in km
	double position[3];
	///ECI velocity in km/s
	double velocity[3];

	///Latitude in radians, northing/easting
	double latitude;
	///Longitude in radians, northing/easting
	double longitude;
	///Altitude in km
	double altitude;
	///Footprint diameter in km
	double footprint;
	///Whether satellite is eclipsed by the earth
	int eclipsed;
	///Eclipse depth
	double eclipse_depth;
	///Orbital phase (mean anomaly)
	double phase;
	///The current number of revolutions around Earth
	long revolutions;

	///Current inclination (from xinck within sgp4/sdp4)
	double inclination;
	///Current right ascension of the ascending node (from xnodek within sgp4/sdp4)
	double right_ascension;
	///Current argument of perigee (from omgadf within sgp4/sdp4)
	double argument_of_perigee;
};

/**
 * Main prediction function. Predict satellite orbit at given time. 
 * \param orbital_elements Orbital elements
 * \param x Predicted orbit
 * \param time Julian day in UTC
 * \return 0 if everything went fine
 * \copyright GPLv2+
 **/
int predict_orbit(const predict_orbital_elements_t *orbital_elements, struct predict_position *x, predict_julian_date_t time);

/**
 * Find whether an orbit is geosynchronous.
 *
 * This function uses the definition of geosynchronous orbits found in
 * "Classification of geosynchronous objects", Issue 17, 28 March 2015, from the
 * European Space Agency:
 *
 * - Eccentricity smaller than 0.2
 * - Mean motion between 0.9 and 1.1
 * - Inclination lower than 70 degrees
 *
 * The function is mainly used internally for avoiding long iteration loops in
 * functions like predict_at_max_elevation() and predict_next_aos(). The wider
 * definition of a geosynchronous orbits is appropriate here. The definition of
 * a geostationary satellite would be stricter, but is not considered here.
 *
 * \param orbital_elements Orbital elements
 * \return true if orbit is geosynchronous, false otherwise
 **/
bool predict_is_geosynchronous(const predict_orbital_elements_t *orbital_elements);

/** 
 * Get apogee of satellite orbit. 
 *
 * \param x Orbital elements
 * \return Apogee of orbit
 * \copyright GPLv2+
 **/
double predict_apogee(const predict_orbital_elements_t *x);

/**
 * Get perigee of satellite orbit. 
 *
 * \param x Orbital elements
 * \return Perigee of orbit
 * \copyright GPLv2+
 **/
double predict_perigee(const predict_orbital_elements_t *x);

/**
 * Find whether an AOS can ever happen on the given latitude. 
 *
 * \param x Orbital elements
 * \param latitude Latitude of ground station in radians
 * \return true if AOS can happen, otherwise false
 * \copyright GPLv2+
 **/
bool predict_aos_happens(const predict_orbital_elements_t *x, double latitude);

/**
 * Observation point/ground station (QTH).
 **/
typedef struct {
	///Observatory name
	char name[128];
	///Latitude (WGS84, radians)
	double latitude;
	///Longitude (WGS84, radians)
	double longitude;
	///Altitude (WGS84, meters)
	double altitude;
} predict_observer_t;

/**
 * Data relevant for a relative observation of an orbit or similar with respect to an observation point.
 **/
struct predict_observation {
	///UTC time                
	predict_julian_date_t time;                       
	///Azimuth angle (rad)      
	double azimuth;
	///Azimuth angle rate (rad/s)
	double azimuth_rate;
	///Elevation angle (rad)                           
	double elevation;
	///Elevation angle rate (rad/s)
	double elevation_rate;
	///Range (km) 
	double range;                        
	///Range vector                    
	double range_x, range_y, range_z; 
	///Range velocity (km/s) 
	double range_rate;      
	///Visibility status, whether satellite can be seen by optical means.
	///The satellite is defined to be visible if:
	//	- The satellite is in sunlight
	//	- The satellite is above the horizon
	//	- The sky is dark enough (sun elevation is below a fixed threshold)
	bool visible;
};

/**
 * Create observation point (QTH).
 *
 * \param name Name of observation point
 * \param lat Latitude in radians (easting/northing)
 * \param lon Longitude in radians (easting/northing)
 * \param alt Altitude in meters
 * \return Allocated observation point
 **/
predict_observer_t *predict_create_observer(const char *name, double lat, double lon, double alt);

/** 
 * Free observer.
 *
 * \param obs Observer to be freed.
 **/
void predict_destroy_observer(predict_observer_t *obs);

/** 
 * Find relative position of satellite with respect to an observer. Calculates range, azimuth, elevation and relative velocity.
 *
 * \param observer Point of observation
 * \param orbit Satellite orbit
 * \param obs Return of object for position of the satellite relative to the observer.
 * \copyright GPLv2+
 **/
void predict_observe_orbit(const predict_observer_t *observer, const struct predict_position *orbit, struct predict_observation *obs);

/**
 * Estimate relative position of the moon.
 *
 * \param observer Point of observation
 * \param time Time of observation
 * \param obs Return object for position of the moon relative to the observer
 * \copyright GPLv2+
 **/
void predict_observe_moon(const predict_observer_t *observer, predict_julian_date_t time, struct predict_observation *obs);

/**
 * Calculate right ascension of the moon.
 *
 * \param time Time
 * \return RA in radians
 **/
double predict_moon_ra(predict_julian_date_t time);

/**
 * Calculate declination of the moon.
 *
 * \param time Time
 * \return Declination in radians
 **/
double predict_moon_declination(predict_julian_date_t time);

/**
 * Calculate the greenwich hour angle (longitude) of the moon.
 *
 * \param time Time
 * \return GHA in radians
 * \copyright GPLv2+
 **/
double predict_moon_gha(predict_julian_date_t time);

/**
 * Estimate relative position of the sun.
 *
 * \param observer Point of observation
 * \param time Time of observation
 * \param obs Return object for position of the sun relative to the observer
 * \copyright GPLv2+
 **/
void predict_observe_sun(const predict_observer_t *observer, predict_julian_date_t time, struct predict_observation *obs);

/**
 * Calculate right ascension of the sun.
 *
 * \param observer Point of observation
 * \param time Time of observation
 * \return RA in radians
 **/
double predict_sun_ra(predict_julian_date_t time);

/**
 * Calculate declination of the sun.
 *
 * \param observer Point of observation
 * \param time Time of observation
 * \return Declination in radians
 **/
double predict_sun_declination(predict_julian_date_t time);

/**
 * Calculate the greenwich hour angle (longitude) of the sun.
 *
 * \param time Time
 * \return GHA in radians
 * \copyright GPLv2+
 **/
double predict_sun_gha(predict_julian_date_t time);

/** 
 * Find next acquisition of signal (AOS) of satellite (when the satellite rises above the horizon). Ignores previous AOS of current pass if the satellite is in range at the start time. 
 *
 * \param observer Point of observation
 * \param orbital_elements Orbital elements
 * \param start_time Start time for AOS search
 * \return Observation of the AOS
 * \copyright GPLv2+
 **/
struct predict_observation predict_next_aos(const predict_observer_t *observer, const predict_orbital_elements_t *orbital_elements, predict_julian_date_t start_time);

/** 
 * Find next loss of signal (LOS) of satellite (when the satellite goes below the horizon). Finds LOS of the current pass if the satellite currently is in range, finds LOS of next pass if not.
 *
 * \param observer Point of observation
 * \param orbital_elements Orbital elements
 * \param start_time Start time for LOS search
 * \return Observation of the LOS
 * \copyright GPLv2+
 **/
struct predict_observation predict_next_los(const predict_observer_t *observer, const predict_orbital_elements_t *orbital_elements, predict_julian_date_t start_time);

/**
 * Find maximum elevation of next or current pass.
 *
 * \param observer Ground station
 * \param orbital_elements Orbital elements of satellite
 * \param start_time Search time. If elevation is negative, max elevation is sought from the start_time and on. If elevation is positive, max elevation is searched for within the current pass
 * \return Observed properties at maximum elevation
 **/
struct predict_observation predict_at_max_elevation(const predict_observer_t *observer, const predict_orbital_elements_t *orbital_elements, predict_julian_date_t start_time);

/**
 * Calculate doppler shift of a given downlink frequency with respect to an observer.
 *
 * \param observation Observation of a satellite orbit
 * \param downlink_frequency Downlink frequency of the satellite
 * \return The frequency difference from the original frequency
 * \copyright GPLv2+
 **/
double predict_doppler_shift(const struct predict_observation *observation, double downlink_frequency);

/**
 * Calculate squint angle for satellite, i.e. angle between the satellite antenna and the QTH antenna.
 *
 * \param observer Point of observation
 * \param orbit Current state of satellite orbit
 * \param alon Attitude longitude in radians (describes orientation of the satellite at apogee)
 * \param alat Attidue latitude in radians (see above)
 * \return Squint angle in radians. Will output nan if the satellite is not an SDP4 satellite
 * \copyright GPLv2+
 **/
double predict_squint_angle(const predict_observer_t *observer, const struct predict_position *orbit, double alon, double alat);

/*!
 * \brief Calculate refraction angle.
 *
 * This function assumes atmospheric pressure of 101.0kPa and temperature 10deg celsius.
 *
 * \param el True elevation angle (rad).
 *
 * \return Refraction angle (rad).
 */
double predict_refraction(double el);

/*!
 * \brief Calculate refraction angle.
 *
 * Corrects for different atmospheric pressure and temperature.
 *
 * \param el True elevation angle in rads.
 * \param pressure Atmospheric pressure in kPa.
 * \param temp Temperature in deg celsius.
 *
 * \return Refraction angle (rad).
 */
double predict_refraction_ext(double el, double pressure, double temp);

/*!
 * \brief Calculate refraction angle from apparent elevation.
 *
 * This function assumes atmospheric pressure of 101.0kPa and temperature 10deg celsius.
 *
 * \param apparent_el Apparent elevation angle (rad).
 *
 * \return Refraction angle (rad).
 */
double predict_refraction_from_apparent(double apparent_el);

/*!
 * \brief Calculate refraction angle from apparent elevation.
 *
 * Corrects for different atmospheric pressure and temperature.
 *
 * \param apparent_el Apparent elevation angle (rad).
 * \param pressure Atmospheric pressure in kPa.
 * \param temp Temperature in deg celsius.
 *
 * \return Refraction angle (rad).
 */
double predict_refraction_from_apparent_ext(double apparent_el, double pressure, double temp);

/*!
 * \brief Calculate refraction rate of change.
 *
 * \param el True elevation angle (rad).
 * \param el_rate Rate of change of true elevation angle (rad/s).
 *
 * \return Refraction rate of change (rad/s).
 */
double predict_refraction_rate(double el, double el_rate);

/*!
 * \brief Calculate refraction rate of change.
 *
 * Corrects for different atmospheric pressure and temerature.
 *
 * \param el True elevation angle (rad).
 * \param el_rate Rate of change of true elevation angle (rad/s).
 * \param pressure Atmospheric pressure in kPa.
 * \param temp Temperature in deg celsius.
 *
 * \return Apparent elevation (rad).
 */
double predict_refraction_rate_ext(double el, double el_rate, double pressure, double temp);

/*!
 * \brief Calculate apparent elevation from true elevation.
 *
 * \param el True elevation angle (rad).
 *
 * \return Apparent elevation (rad).
 */
double predict_apparent_elevation(double el);

/*!
 * \brief Calculate apparent elevation from true elevation.
 *
 * Corrects for different atmospheric pressures and temperatures.
 *
 * \param el True elevation angle (rad).
 * \param pressure Atmospheric pressure (kPa).
 * \param temp Temperature (deg C).
 *
 * \return Apparent elevation (rad).
 */
double predict_apparent_elevation_ext(double el, double pressure, double temp);

/*!
 * \brief Calculate apparent elevation rate.
 *
 * \param el True elevation angle (rad).
 * \param el_rate Rate of change of true elevation angle (rad/s).
 *
 * \return Rate of change of apparent elevation (rad/s).
 */
double predict_apparent_elevation_rate(double el, double el_rate);

/*!
 * \brief Calculate apparent elevation rate.
 *
 * Corrects for different atmospheric pressures and temperatures.
 *
 * \param el True elevation angle (rad).
 * \param el_rate Rate of change of true elevation angle (rad/s).
 * \param pressure Atmospheric pressure (kPa).
 * \param temp Temperature (deg C).
 *
 * \return Rate of change of apparent elevation (rad/s).
 */
double predict_apparent_elevation_rate_ext(double el, double el_rate, double pressure, double temp);

#ifdef __cplusplus
}
#endif

#endif //_PREDICT_H_

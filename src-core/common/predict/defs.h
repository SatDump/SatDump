#ifndef _PREDICT_DEFS_H_
#define _PREDICT_DEFS_H_

#include <float.h>

/** \name Geosynchronous orbit definitions
 *  Requirements for an orbit to be called geosynchronous.
 */
///@{
///lower mean motion for geosynchronous satellites
#define GEOSYNCHRONOUS_LOWER_MEAN_MOTION 0.9
///upper mean motion for geosynchronous satellites
#define GEOSYNCHRONOUS_UPPER_MEAN_MOTION 1.1
///upper eccentricity for geosynchronous satellites
#define GEOSYNCHRONOUS_ECCENTRICITY_THRESHOLD 0.2
///upper inclination for geosynchronous satellites
#define GEOSYNCHRONOUS_INCLINATION_THRESHOLD_DEGREES 70
///@}

/** \name Mathematical constants
 *  Mathematical convenience constants used by sgp4/sdp4 and related routines.
 */
///@{
/// pi
#define PI		3.14159265358979323846
/// pi/2
#define PI_HALF 	1.57079632679489656
/// 2*pi
#define TWO_PI          6.28318530717958623
/// 3*pi/2
#define THREE_PI_HALF	4.71238898038468967
/// 2/3
#define TWO_THIRD	6.6666666666666666E-1
///@}

/** \name Time constants
 *  Constants used for time conversions.
 */
///@{
///Number of minutes per day, XMNPDA in spacetrack report #3
#define MINUTES_PER_DAY	1.44E3
///Number of seconds per day
#define SECONDS_PER_DAY	8.6400E4
///Difference between libpredict's predict_julian_date_t and the julian time format used in some of the internal functions
#define JULIAN_TIME_DIFF 2444238.5
///@}

/** \name Physical properties
 *  General physical properties and definitions.
 */
///@{
///J3 Harmonic (WGS '72), XJ3 in spacetrack report #3
#define J3_HARMONIC_WGS72 			-2.53881E-6
///WGS 84 Earth radius km, XKMPER in spacetrack report #3
#define EARTH_RADIUS_KM_WGS84			6.378137E3
///Flattening factor
#define FLATTENING_FACTOR 			3.35281066474748E-3
///Earth rotations per siderial day
#define EARTH_ROTATIONS_PER_SIDERIAL_DAY	1.00273790934
///Solar radius in kilometers (IAU 76)
#define SOLAR_RADIUS_KM 			6.96000E5
///Astronomical unit in kilometers (IAU 76)
#define ASTRONOMICAL_UNIT_KM 			1.49597870691E8
///Upper elevation threshold for nautical twilight
#define NAUTICAL_TWILIGHT_SUN_ELEVATION 	-12.0
///Speed of light in vacuum
#define SPEED_OF_LIGHT				299792458.0
///Angular velocity of Earth in radians per seconds
#define EARTH_ANGULAR_VELOCITY			7.292115E-5
///@}

/** \name Iteration constants
 *  Constants used in iteration functions like predict_max_elevation(),
 *  predict_next_aos() and predict_next_los().
 */
///@{
///Threshold used for fine-tuning of AOS/LOS
#define AOSLOS_HORIZON_THRESHOLD 	0.3
///Threshold used for comparing lower and upper brackets in find_max_elevation
#define MAXELE_TIME_EQUALITY_THRESHOLD 	FLT_EPSILON
///Maximum number of iterations in find_max_elevation
#define MAXELE_MAX_NUM_ITERATIONS 	10000
///@}

/** \name General spacetrack report #3 constants
 *  These constants are also used outside of SGP4/SDP4 code.  The
 *  constants/symbols are defined on page 76 to page 78 in the report.
 */
///@{
///k_e = sqrt(Newton's universal gravitational * mass of the Earth), in units (earth radii per minutes)^3/2
#define XKE		7.43669161E-2
///Corresponds to 1/2 * J_2 * a_E^2. J_2 is the second gravitational zonal harmonic of Earth, a_E is the equatorial radius of Earth.
#define CK2		5.413079E-4
///@}

/** \name Specific spacetrack report #3 constants
 *  These constants are only used by SGP4/SDP4 code.  The constants/symbols are
 *  defined on page 76 to page 78 in the report.
 */
///@{
///Shorthand for 10^-6.
#define E6A		1.0E-6
///Distance units / Earth radii.
#define AE		1.0
///Corresponds to -3/8 * J_4 * a_E^4, where J_4 is the fourth gravitational zonal harmonic of Earth.
#define CK4		6.209887E-7
///Parameter for SGP4/SGP8 density function.
#define S_DENSITY_PARAM	1.012229
///Corresponds to (q_0 - s)^4 in units (earth radii)^4, where q_0 and s are parameters for the SGP4/SGP8 density function.
#define QOMS2T		1.880279E-09
///@}

/** \name Constants in deep space subroutines
 *  Not defined in spacetrack report #3.
 *
 *  The constants might originally be defined in Hujsak (1979) and/or Hujsak
 *  and Hoots (1977), but this is unavailable on the Internet. Reiterated in F.
 *  R.  Hoots, P. W. Schumacher and R. A. Glober, "A HISTORY OF ANALYTICAL
 *  ORBIT MODELING IN THE UNITED STATES SPACE SURVEILLANCE SYSTEM", 2004. Page
 *  numbers below refer to this article.
 */
///@{
///Solar mean motion (n_s in units radians/minute, p. 29)
#define ZNS		1.19459E-5
///Solar perturbation coefficient (C_s in units radians/minute, p. 29)
#define C1SS		2.9864797E-6
///Solar eccentricity (e_s, p. 29)
#define ZES		1.675E-2
///Lunar mean motion (n_m in units radians/minute, p. 29)
#define ZNL		1.5835218E-4
///Lunar perturbation coefficient (C_m in units radians/minute, p. 29)
#define C1L		4.7968065E-7
///Lunar eccentricity (e_m, p. 29)
#define ZEL		5.490E-2
///Cosine of the solar inclination (not defined directly in the paper, but corresponds with cos(I_s) with I_s as the solar inclination on p. 29)
#define ZCOSIS		9.1744867E-1
///Sine of the solar inclination (sin(I_s), I_s on p. 29. See comment above)
#define ZSINIS		3.9785416E-1
///Corresponds to sin(\omega_s) (\omega_s defined on p. 29, no description. See comment above)
#define ZSINGS		-9.8088458E-1
///Corresponds to cos(\omega_s) (\omega_s defined on p. 29, no description. See comment above)
#define ZCOSGS		1.945905E-1
///Constants for one-day resonance conditions, satellite-independent for 1-day period satellites (Initialization of resonance effects of Earth gravity, Q_22, Q_31 and Q_33, p. 31)
#define Q22		1.7891679E-6
///See above
#define Q31		2.1460748E-6
///See above
#define Q33		2.2123015E-7
///Constants for secular update for resonance effects of Earth gravity (G_22, G_32, G_44, G_52 and G_54, p. 36)
#define G22		5.7686396
///See above
#define G32		9.5240898E-1
///See above
#define G44		1.8014998
///See above
#define G52		1.0508330
///See above
#define G54		4.4108898
///Constants for 1/2-day resonance conditions, satellite-independent for 1/2-day period satellites (Initialization for resonance effects of Earth gravity, sqrt(C_ij^2 + S_ij^2) where ij = 22, 32, 44, 52 and 54, p. 32)
#define ROOT22		1.7891679E-6
///See above
#define ROOT32		3.7393792E-7
///See above
#define ROOT44		7.3636953E-9
///See above
#define ROOT52		1.1428639E-7
///See above
#define ROOT54		2.1765803E-9
///The time-derivative of the Greenwich hour angle in radians per minute (\dot{\theta}, used on p. 36. Not directly defined in report, but values and naming are consistent with this)
#define THDT           4.3752691E-3
///@}

#endif

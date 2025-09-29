#include "common/big_endian.h"

#ifdef _WIN32
#pragma pack(push, 1)
#endif

/**
 * @brief S-VISSR Orbit And Attitude subcommunication block contents.
 * Docstrings contain:
 * - Encoding value: R*X.Y = X-byte big endian integer, 10^-Y for the value | I*2 - 2-byte integet | BCD*6 - 6-byte byte coded decimal
 * - 'fixed value' if the value is constant
 * - A description of the value
 * - The unit to be used '[unit]'
 */
struct OrbitAndAttitudeData
{
    be_uint48_t IMAGE_START_TIME;                        /* R*6.8, [MJD] */
    be_uint32_t VIS_STEPPING_LINE;                       /* R*4.8, VIS channel stepping angle along line, [rad] */
    be_uint32_t IR_STEPPING_LINE;                        /* R*4.8, IR channel stepping angle along line, [rad] */
    be_uint32_t VIS_STEPPING_PIXEL;                      /* R*4.10, VIS channel stepping along pixel, [rad] */
    be_uint32_t IR_STEPPING_PIXEL;                       /* R*4.10, IR channel stepping along pixel, [rad] */
    be_uint32_t VIS_CENTER_LINE;                         /* R*4.4 VIS channel center line number of the VISSR frame */
    be_uint32_t IR1_CENTER_LINE;                         /* R*4.4 IR1 channel center line number of the VISSR frame */
    be_uint32_t VIS_CENTER_PIXEL;                        /* R*4.4, VIS channel center pixel number of the VISSR frame */
    be_uint32_t IR1_CENTER_PIXEL;                        /* R*4.4, IR1 channel center pixel number of the VISSR frame */
    be_uint32_t VIS_SENSOR_NUMBER;                       /* R*4.0 Number of VIS channel sensors */
    be_uint32_t IR_SENSOR_NUMBER;                        /* R*4.0 Number of IR channel sensors */
    be_uint32_t VIS_TOTAL_LINE_NUMBER;                   /* R*4.0 Total number of VIS channel lines in VISSR frame */
    be_uint32_t IR_TOTAL_LINE_NUMBER;                    /* R*4.0 Total number of IR channel lines in VISSR frame */
    be_uint32_t VIS_LINE_PX;                             /* R*4.0 Pixels per VIS line */
    be_uint32_t IR_LINE_PX;                              /* R*4.0 Pixel per IR line */
    be_uint32_t VISSR_MISALIGNMENT_ANGLE_X_AXIS;         /* R*4.10, VISSR misalignment angle around the x axis, [rad] */
    be_uint32_t VISSR_MISALIGNMENT_ANGLE_Y_AXIS;         /* R*4.10, VISSR misalignment angle around the y axis, [rad] */
    be_uint32_t VISSR_MISALIGNMENT_ANGLE_Z_AXIS;         /* R*4.10, VISSR misalignment angle around the z axis, [rad] */
    be_uint32_t VMM_R1_C1;                               /* R*4.7,  fixed value, VISSR misalignment matrix row 1 column 1 */
    be_uint32_t VMM_R2_C1;                               /* R*4.10, fixed value, VISSR misalignment matrix row 2 column 1 */
    be_uint32_t VMM_R3_C1;                               /* R*4.10, fixed value, VISSR misalignment matrix row 3 column 1 */
    be_uint32_t VMM_R1_C2;                               /* R*4.10, fixed value, VISSR misalignment matrix row 1 column 2 */
    be_uint32_t VMM_R2_C2;                               /* R*4.7,  fixed value, VISSR misalignment matrix row 2 column 2 */
    be_uint32_t VMM_R3_C2;                               /* R*4.10, fixed value, VISSR misalignment matrix row 3 column 2 */
    be_uint32_t VMM_R1_C3;                               /* R*4.10, fixed value, VISSR misalignment matrix row 1 column 3 */
    be_uint32_t VMM_R2_C3;                               /* R*4.10, fixed value, VISSR misalignment matrix row 2 column 3 */
    be_uint32_t VMM_R3_C3;                               /* R*4.7,  fixed value, VISSR misalignment matrix row 3 column 3 */
    be_uint32_t IR2_channel_center_line_number_of_frame; /* R*4.4 IR 2 channel's center line number in the VISSR frame */
    be_uint32_t IR3_channel_center_line_number_of_frame; /* R*4.4 IR 3 channel's center line number in the VISSR frame */
    uint8_t spare[10];                                   // The above two values are repeated again here, I just added them to the 2 byte spare to be skipped
    be_uint32_t RATIO_OF_CIRCUMFERENCE;                  /* R*4.7, fixed value, ratio of circumference - pi */
    be_uint32_t RATIO_OF_CIRCUMFERENCE_1;                /* R*4.9, fixed value, pi/180 */
    be_uint32_t RATIO_OF_CIRCUMFERENCE_2;                /* R*4.9, fixed value, 180/pi */
    be_uint32_t EARTH_RADIUS;                            /* R*4.1, Equatorial radius, fixed value, [m] */
    be_uint32_t EARTH_OBLATENESS;                        /* R*4.10, fixed value */
    be_uint32_t EARTH_ECCENTRICITY;                      /* R*4.9, fixed value */
    be_uint32_t VISSR_SUN_SENSOR_ANGLE;                  /* R*4.8, fixed value, Angle between VISSR and view of sun sensor at data acquisition start, [rad] */

    // Orbit params
    be_uint48_t ORBITAL_PARAM_EPOCH;      /* R*6.8, Epoch of orbital parameters, Orbit parameters in mean J2000, [MJD] */
    be_uint48_t SEMI_MAJOR_AXIS;          /* R*6.8, Orbit parameters in mean J2000, [km] */
    be_uint48_t ECCENTRICITY;             /* R*6.10, Orbit parameters in mean J2000 */
    be_uint48_t INCLINATION;              /* R*6.8, Orbit parameters in mean J2000, [deg] */
    be_uint48_t LONGITUDE_ASCENDING_NODE; /* R*6.8, Longitude of ascending node, Orbit parameters in mean J2000, [deg] */
    be_uint48_t PERIGEE;                  /* R*6.8, Argument of perigee, Orbit parameters in mean J2000, [deg] */
    be_uint48_t MEAN_ANOMALY;             /* R*6.8, Orbit parameters in mean J2000, [deg] */
    be_uint48_t SSP_LONGITUDE;            /* R*6.6, Sub-satellite East Longitude, Orbit parameters in mean J2000, [deg] */
    be_uint48_t SSP_LATITUDE;             /* R*6.6, Sub-satellite North Latitude, Orbit parameters in mean J2000, [deg] */

    // Attitude params
    be_uint48_t ATTITUDE_PARAM_EPOCH;              /* R*6.8, Epoch of attitude parameters, Attitude parameters in mean J2000, [MJD] */
    be_uint48_t ALPHA_Z_AXIS_SAT_SPIN_ON_YZ_PLANE; /* R*6.8, Angle between Z-axis and Satellite Spin Axis projected on yz-plane α, Attitude parameters in mean J2000, [rad] */
    be_uint48_t ALPHA_CHANGE_RATE;                 /* R*6.15, Change-rate of α, Attitude parameters in mean J2000, [rad/sec] */
    be_uint48_t DELTA_SAT_SPIN_YZ_PLANE;           /* R*6.11, Angle between Satellite Spin Axis and yz-plane δ, Attitude parameters in mean J2000, [rad] */
    be_uint48_t DELTA_CHANGE_RATE;                 /* R*6.15, Change-rate of δ, Angle between Satellite Spin Axis and yz-plane δ, Attitude parameters in mean J2000, [rad/sec] */
    be_uint48_t MEAN_SPIN_RATE;                    /* R*6.8, Estimated daily mean spin rate, [rpm] */
    uint8_t spare2[10];

    uint8_t ATTITUDE_PREDICTION_SUBBLOCKS[10][64]; /* 10x 64-byte attitude prediction sub block*/
    uint8_t ORBIT_PREDICTION_SUBBLOCKS[8][256];    /* 8x 256-byte orbit prediction sub block */
    be_uint48_t FIRST_ATTITUDE_PREDICTION;         /* R*6.8, Time of the first attitude prediction record, [MJD]*/
    be_uint48_t LATEST_ATTITUDE_PREDICTION;        /* R*6.8, Time of the last attitude prediction record, [MJD]*/
    be_uint48_t ATTITUDE_PREDICTION_INTERVAL;      /* R*6.8, fixed value (0.00347222), Interval time of attitude prediction data, [MJD]*/
    uint16_t ATTITUDE_PREDICTION_NUMBER;           /* I*2 Number of attitude prediction data block, [=10] */
    be_uint48_t FIRST_ORBITAL_PREDICTION;          /* R*6.8, Time of the first orbit prediction record, [MJD]*/
    be_uint48_t LATEST_ORBITAL_PREDICTION;         /* R*6.8, Time of the last orbit prediction record, [MJD]*/
    be_uint48_t ORBITAL_PREDICTION_INTERVAL;       /* R*6.8, fixed value (0.00347222), Interval time of orbit prediction data, [MJD]*/
    uint32_t ORBITAL_PREDICTION_COUNT;             /* I*2 Number of attitude prediction data block, [=8] */
    uint8_t spare3[216];
}

#ifdef _WIN32
;
#else
__attribute__((packed));
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif

/**
 * @brief 64-byte sub block that is present 10 times, contains attitude prediction data
 * 
 */
struct AttitudePredictionSubBlock
{
    be_uint48_t PREDICTION_TIME;                   /* R*6.8, UTC time when attitude prediction is processed, [MJD] */
    be_uint48_t PREDICTION_TIME_BCD;               /* BCD*6, time when attitude prediction is processed, [BCD] */
    be_uint48_t Z_AXIS_SAT_SPIN_ON_YZ_PLANE_ANGLE; /* R*6.8, Angle between z-axis and satellite spin axis projected on yz-plane in mean of 1950.0 coordinates, mean J2000, [rad] */
    be_uint48_t SAT_SPIN_YZ_PLANE_ANGLE;           /* R*6.11, Angle between satelltie spin axis and yz-plane, [rad] */
    be_uint48_t DIHEDRAL_ANGLE;                    /* R*6.8, Dihedral angle between the Sun and Earth measured clockwise viewing from North, [rad]*/
    be_uint48_t SPIN_RATE;                         /* R*6.8, Spin speed of satellite, [rpm] */
    be_uint48_t RIGHT_ASCENSION_SPIN_AXIS;         /* R*6.8, Right ascension of satellite spin axis on the sat orbit plane coordinate sysetm, [rad] */
    be_uint48_t DECLINATION_SPIN_AXIS;             /* R*6.8, Declination of satellite spin axis on the sat orbit plane coordinate sysetm, [rad]*/
    uint8_t spare[16];
};

/**
 * @brief 256-byte sub block that is present 8 times, contains orbit prediction data
 * 
 */
struct OrbitPredictionSubBlock
{
    be_uint48_t PREDICTION_TIME_MJD;      /* R*6.8, UTC time when orbit prediction is processed, [MJD] */
    be_uint48_t PREDICTION_TIME_BCD;      /* BCD*6, time when orbit prediction is processed, [BCD] */
    be_uint48_t X_COMP;                   /* R*6.8,  X component of the satellite position in mean J2000, [m] */
    be_uint48_t Y_COMP;                   /* R*6.8,  Y component of the satellite position in mean J2000, [m] */
    be_uint48_t Z_COMP;                   /* R*6.8,  Z component of the satellite position in mean J2000, [m] */
    be_uint48_t X_COMP_SPEED;             /* R*6.8,  X component of the satellite position in mean J2000, [m/s] */
    be_uint48_t Y_COMP_SPEED;             /* R*6.8,  Y component of the satellite position in mean J2000, [m/s] */
    be_uint48_t Z_COMP_SPEED;             /* R*6.8,  Z component of the satellite position in mean J2000, [m/s] */
    be_uint48_t X_COMP_EARTH_FIXED;       /* R*6.6,  X component of the satellite position in earth-fixed coordinates, [m] */
    be_uint48_t Y_COMP_EARTH_FIXED;       /* R*6.6,  Y component of the satellite position in earth-fixed coordinates, [m] */
    be_uint48_t Z_COMP_EARTH_FIXED;       /* R*6.6,  Z component of the satellite position in earth-fixed coordinates, [m] */
    be_uint48_t X_COMP_EARTH_FIXED_SPEED; /* R*6.10, X component of the satellite position in earth-fixed coordinates, [m/s] */
    be_uint48_t Y_COMP_EARTH_FIXED_SPEED; /* R*6.10, Y component of the satellite position in earth-fixed coordinates, [m/s] */
    be_uint48_t Z_COMP_EARTH_FIXED_SPEED; /* R*6.10, Z component of the satellite position in earth-fixed coordinates, [m/s] */
    be_uint48_t GREENWHICH_SIDEREAL;      /* R*6.8, Greenwhich sidereal time in true of data coordinates, [deg] */
    be_uint48_t RATS_J2;                  /* R*6.8, Right ascension from the satellite to the sun in mean of J2000 coordinates, [deg] */
    be_uint48_t DSTS_J2;                  /* R*6.8, Declination from the satellite to the sun in mean of J2000 coordinates, [deg] */
    be_uint48_t RATS_EF;                  /* R*6.8, Right ascension from the satellite to the sun in earth-fixed coordinates, [deg] */
    be_uint48_t DSTS_EF;                  /* R*6.8, Declination from the satellite to the sun in earth-fixed coordinates, [deg] */
    uint8_t spare[14];
    be_uint48_t NPM_R1_C1;     /* R6*12 Nutation and precession matrix row 1 column 1 */
    be_uint48_t NPM_R2_C1;     /* R6*14 Nutation and precession matrix row 2 column 1 */
    be_uint48_t NPM_R3_C1;     /* R6*14 Nutation and precession matrix row 3 column 1 */
    be_uint48_t NPM_R1_C2;     /* R6*14 Nutation and precession matrix row 1 column 2 */
    be_uint48_t NPM_R2_C2;     /* R6*12 Nutation and precession matrix row 2 column 2 */
    be_uint48_t NPM_R3_C2;     /* R6*16 Nutation and precession matrix row 3 column 2 */
    be_uint48_t NPM_R1_C3;     /* R6*12 Nutation and precession matrix row 1 column 3 */
    be_uint48_t NPM_R2_C3;     /* R6*16 Nutation and precession matrix row 2 column 3 */
    be_uint48_t NPM_R3_C3;     /* R6*12 Nutation and precession matrix row 3 column 3 */
    be_uint48_t SSP_LATITUDE;  /* R*6.8, North Latitude, [deg] */
    be_uint48_t SSP_LONGITUDE; /* R*6.8, East Longitude, [deg] */
    be_uint48_t ALTITUDE;      /* R*6.6, Height of sat above Earth surface, [m] */
    uint8_t spare2[56];
};

#include <cstdint>

/**
 * @file svissr_blocks.h
 * @brief Definitions for subcommunication blocks
 */


#ifdef _WIN32
#pragma pack(push, 1)
#endif
/**
 * @brief Takes 6 bytes, creates a signed 48-bit integer in big endian. Stored in a 64 bit signed integer
 *
 */
struct svissr_r6_t
{
    uint8_t bytes[6];

    int64_t value() const
    {
        uint64_t raw = ((uint64_t)bytes[0] << 40) | ((uint64_t)bytes[1] << 32) | ((uint64_t)bytes[2] << 24) | ((uint64_t)bytes[3] << 16) | ((uint64_t)bytes[4] << 8) | ((uint64_t)bytes[5]);

        // sign bit is set to zero
        if (bytes[0] & 128)
        {
            raw &= 0x7fffffffffff;
            return -((int64_t)raw);
        }

        return (int64_t)raw;
    }
    operator int64_t() const { return value(); }
}

#ifdef _WIN32
;
#else
__attribute__((packed));
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif

#ifdef _WIN32
#pragma pack(push, 1)
#endif
/**
 * @brief Creates a signed 4-byte integer from big-endian bytes
 *
 */
struct svissr_r4_t
{
    uint8_t bytes[4];

    int32_t value() const
    {
        uint32_t raw = ((uint32_t)bytes[0] << 24) | ((uint64_t)bytes[1] << 16) | ((uint64_t)bytes[2] << 8) | ((uint64_t)bytes[3]);

        // sign bit is set to zero
        if (bytes[0] & 128)
        {
            raw &= 0x7fffffff;
            return -((int32_t)raw);
        }

        return (int32_t)raw;
    }
    operator int32_t() const { return value(); }
}

#ifdef _WIN32
;
#else
__attribute__((packed));
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif

#ifdef _WIN32
#pragma pack(push, 1)
#endif

/**
 * @brief S-VISSR Orbit And Attitude subcommunication block contents. All integers are signed using the MSB bit, NOT TWO-COMPLEMENT!!!
 * Docstrings contain:
 * - Encoding value: R*X.Y = X-byte big endian integer, 10^-Y for the value | I*2 - 2-byte integer | BCD*6 - 6-byte byte coded decimal
 * - 'fixed value' if the value is constant
 * - A description of the value
 * - The unit to be used '[unit]'
 */
struct OrbitAndAttitudeData
{
    svissr_r6_t IMAGE_START_TIME;                        /* R*6.8, [MJD] */
    svissr_r4_t VIS_STEPPING_LINE;                       /* R*4.8, VIS channel stepping angle along line, [rad] */
    svissr_r4_t IR_STEPPING_LINE;                        /* R*4.8, IR channel stepping angle along line, [rad] */
    svissr_r4_t VIS_STEPPING_PIXEL;                      /* R*4.10, VIS channel stepping along pixel, [rad] */
    svissr_r4_t IR_STEPPING_PIXEL;                       /* R*4.10, IR channel stepping along pixel, [rad] */
    svissr_r4_t VIS_CENTER_LINE;                         /* R*4.4 VIS channel center line number of the VISSR frame */
    svissr_r4_t IR1_CENTER_LINE;                         /* R*4.4 IR1 channel center line number of the VISSR frame */
    svissr_r4_t VIS_CENTER_PIXEL;                        /* R*4.4, VIS channel center pixel number of the VISSR frame */
    svissr_r4_t IR1_CENTER_PIXEL;                        /* R*4.4, IR1 channel center pixel number of the VISSR frame */
    svissr_r4_t VIS_SENSOR_NUMBER;                       /* R*4.0 Number of VIS channel sensors */
    svissr_r4_t IR_SENSOR_NUMBER;                        /* R*4.0 Number of IR channel sensors */
    svissr_r4_t VIS_TOTAL_LINE_NUMBER;                   /* R*4.0 Total number of VIS channel lines in VISSR frame */
    svissr_r4_t IR_TOTAL_LINE_NUMBER;                    /* R*4.0 Total number of IR channel lines in VISSR frame */
    svissr_r4_t VIS_LINE_PX;                             /* R*4.0 Pixels per VIS line */
    svissr_r4_t IR_LINE_PX;                              /* R*4.0 Pixel per IR line */
    svissr_r4_t VISSR_MISALIGNMENT_ANGLE_X_AXIS;         /* R*4.10, VISSR misalignment angle around the x axis, [rad] */
    svissr_r4_t VISSR_MISALIGNMENT_ANGLE_Y_AXIS;         /* R*4.10, VISSR misalignment angle around the y axis, [rad] */
    svissr_r4_t VISSR_MISALIGNMENT_ANGLE_Z_AXIS;         /* R*4.10, VISSR misalignment angle around the z axis, [rad] */
    svissr_r4_t VMM_R1_C1;                               /* R*4.7,  fixed value, VISSR misalignment matrix row 1 column 1 */
    svissr_r4_t VMM_R2_C1;                               /* R*4.10, fixed value, VISSR misalignment matrix row 2 column 1 */
    svissr_r4_t VMM_R3_C1;                               /* R*4.10, fixed value, VISSR misalignment matrix row 3 column 1 */
    svissr_r4_t VMM_R1_C2;                               /* R*4.10, fixed value, VISSR misalignment matrix row 1 column 2 */
    svissr_r4_t VMM_R2_C2;                               /* R*4.7,  fixed value, VISSR misalignment matrix row 2 column 2 */
    svissr_r4_t VMM_R3_C2;                               /* R*4.10, fixed value, VISSR misalignment matrix row 3 column 2 */
    svissr_r4_t VMM_R1_C3;                               /* R*4.10, fixed value, VISSR misalignment matrix row 1 column 3 */
    svissr_r4_t VMM_R2_C3;                               /* R*4.10, fixed value, VISSR misalignment matrix row 2 column 3 */
    svissr_r4_t VMM_R3_C3;                               /* R*4.7,  fixed value, VISSR misalignment matrix row 3 column 3 */
    svissr_r4_t IR2_channel_center_line_number_of_frame; /* R*4.4 IR 2 channel's center line number in the VISSR frame */
    svissr_r4_t IR3_channel_center_line_number_of_frame; /* R*4.4 IR 3 channel's center line number in the VISSR frame */
    uint8_t spare[10];                                   // The above two values are repeated again here, I just added them to the 2 byte spare to be skipped
    svissr_r4_t RATIO_OF_CIRCUMFERENCE;                  /* R*4.7, fixed value, ratio of circumference - pi */
    svissr_r4_t RATIO_OF_CIRCUMFERENCE_1;                /* R*4.9, fixed value, pi/180 */
    svissr_r4_t RATIO_OF_CIRCUMFERENCE_2;                /* R*4.6, fixed value, 180/pi */
    svissr_r4_t EARTH_RADIUS;                            /* R*4.1, Equatorial radius, fixed value, [m] */
    svissr_r4_t EARTH_OBLATENESS;                        /* R*4.10, fixed value */
    svissr_r4_t EARTH_ECCENTRICITY;                      /* R*4.9, fixed value */
    svissr_r4_t VISSR_SUN_SENSOR_ANGLE;                  /* R*4.8, fixed value, Angle between VISSR and view of sun sensor at data acquisition start, [rad] */

    // Orbit params
    svissr_r6_t ORBITAL_PARAM_EPOCH;      /* R*6.8, Epoch of orbital parameters, Orbit parameters in mean J2000, [MJD] */
    svissr_r6_t SEMI_MAJOR_AXIS;          /* R*6.8, Orbit parameters in mean J2000, [km] */
    svissr_r6_t ECCENTRICITY;             /* R*6.10, Orbit parameters in mean J2000 */
    svissr_r6_t INCLINATION;              /* R*6.8, Orbit parameters in mean J2000, [deg] */
    svissr_r6_t LONGITUDE_ASCENDING_NODE; /* R*6.8, Longitude of ascending node, Orbit parameters in mean J2000, [deg] */
    svissr_r6_t PERIGEE;                  /* R*6.8, Argument of perigee, Orbit parameters in mean J2000, [deg] */
    svissr_r6_t MEAN_ANOMALY;             /* R*6.8, Orbit parameters in mean J2000, [deg] */
    svissr_r6_t SSP_LONGITUDE;            /* R*6.6, Sub-satellite East Longitude, Orbit parameters in mean J2000, [deg] */
    svissr_r6_t SSP_LATITUDE;             /* R*6.6, Sub-satellite North Latitude, Orbit parameters in mean J2000, [deg] */

    // Attitude params
    svissr_r6_t ATTITUDE_PARAM_EPOCH;              /* R*6.8, Epoch of attitude parameters, Attitude parameters in mean J2000, [MJD] */
    svissr_r6_t ALPHA_Z_AXIS_SAT_SPIN_ON_YZ_PLANE; /* R*6.8, Angle between Z-axis and Satellite Spin Axis projected on yz-plane α, Attitude parameters in mean J2000, [rad] */
    svissr_r6_t ALPHA_CHANGE_RATE;                 /* R*6.15, Change-rate of α, Attitude parameters in mean J2000, [rad/sec] */
    svissr_r6_t DELTA_SAT_SPIN_YZ_PLANE;           /* R*6.11, Angle between Satellite Spin Axis and yz-plane δ, Attitude parameters in mean J2000, [rad] */
    svissr_r6_t DELTA_CHANGE_RATE;                 /* R*6.15, Change-rate of δ, Angle between Satellite Spin Axis and yz-plane δ, Attitude parameters in mean J2000, [rad/sec] */
    svissr_r6_t MEAN_SPIN_RATE;                    /* R*6.8, Estimated daily mean spin rate, [rpm] */
    uint8_t spare2[10];

    uint8_t ATTITUDE_PREDICTION_SUBBLOCKS[10][64]; /* 10x 64-byte attitude prediction sub block*/
    uint8_t ORBIT_PREDICTION_SUBBLOCKS[8][256];    /* 8x 256-byte orbit prediction sub block */
    svissr_r6_t FIRST_ATTITUDE_PREDICTION;         /* R*6.8, Time of the first attitude prediction record, [MJD]*/
    svissr_r6_t LATEST_ATTITUDE_PREDICTION;        /* R*6.8, Time of the last attitude prediction record, [MJD]*/
    svissr_r6_t ATTITUDE_PREDICTION_INTERVAL;      /* R*6.8, fixed value (0.00347222), Interval time of attitude prediction data, [MJD]*/
    int16_t ATTITUDE_PREDICTION_NUMBER;            /* I*2 Number of attitude prediction data block, [=10] */
    svissr_r6_t FIRST_ORBITAL_PREDICTION;          /* R*6.8, Time of the first orbit prediction record, [MJD]*/
    svissr_r6_t LATEST_ORBITAL_PREDICTION;         /* R*6.8, Time of the last orbit prediction record, [MJD]*/
    svissr_r6_t ORBITAL_PREDICTION_INTERVAL;       /* R*6.8, fixed value (0.00347222), Interval time of orbit prediction data, [MJD]*/
    int32_t ORBITAL_PREDICTION_COUNT;              /* I*2 Number of attitude prediction data block, [=8] */
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

#ifdef _WIN32
#pragma pack(push, 1)
#endif

/**
 * @brief 64-byte sub block that is present 10 times, contains attitude prediction data
 *
 */
struct AttitudePredictionSubBlock
{
    svissr_r6_t PREDICTION_TIME;                   /* R*6.8, UTC time when attitude prediction is processed, [MJD] */
    svissr_r6_t PREDICTION_TIME_BCD;               /* BCD*6, time when attitude prediction is processed, [BCD] */
    svissr_r6_t Z_AXIS_SAT_SPIN_ON_YZ_PLANE_ANGLE; /* R*6.8, Angle between z-axis and satellite spin axis projected on yz-plane in mean of 1950.0 coordinates, mean J2000, [rad] */
    svissr_r6_t SAT_SPIN_YZ_PLANE_ANGLE;           /* R*6.11, Angle between satelltie spin axis and yz-plane, [rad] */
    svissr_r6_t DIHEDRAL_ANGLE;                    /* R*6.8, Dihedral angle between the Sun and Earth measured clockwise viewing from North, [rad]*/
    svissr_r6_t SPIN_RATE;                         /* R*6.8, Spin speed of satellite, [rpm] */
    svissr_r6_t RIGHT_ASCENSION_SPIN_AXIS;         /* R*6.8, Right ascension of satellite spin axis on the sat orbit plane coordinate sysetm, [rad] */
    svissr_r6_t DECLINATION_SPIN_AXIS;             /* R*6.8, Declination of satellite spin axis on the sat orbit plane coordinate sysetm, [rad]*/
    uint8_t spare[16];
}

#ifdef _WIN32
;
#else
__attribute__((packed));
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif

#ifdef _WIN32
#pragma pack(push, 1)
#endif

/**
 * @brief 256-byte sub block that is present 8 times, contains orbit prediction data
 *
 */
struct OrbitPredictionSubBlock
{
    svissr_r6_t PREDICTION_TIME_MJD;      /* R*6.8, UTC time when orbit prediction is processed, [MJD] */
    svissr_r6_t PREDICTION_TIME_BCD;      /* BCD*6, time when orbit prediction is processed, [BCD] */
    svissr_r6_t X_COMP;                   /* R*6.6,  X component of the satellite position in mean J2000, [m] */
    svissr_r6_t Y_COMP;                   /* R*6.6,  Y component of the satellite position in mean J2000, [m] */
    svissr_r6_t Z_COMP;                   /* R*6.6,  Z component of the satellite position in mean J2000, [m] */
    svissr_r6_t X_COMP_SPEED;             /* R*6.8,  X component of the satellite position in mean J2000, [m/s] */
    svissr_r6_t Y_COMP_SPEED;             /* R*6.8,  Y component of the satellite position in mean J2000, [m/s] */
    svissr_r6_t Z_COMP_SPEED;             /* R*6.8,  Z component of the satellite position in mean J2000, [m/s] */
    svissr_r6_t X_COMP_EARTH_FIXED;       /* R*6.6,  X component of the satellite position in earth-fixed coordinates, [m] */
    svissr_r6_t Y_COMP_EARTH_FIXED;       /* R*6.6,  Y component of the satellite position in earth-fixed coordinates, [m] */
    svissr_r6_t Z_COMP_EARTH_FIXED;       /* R*6.6,  Z component of the satellite position in earth-fixed coordinates, [m] */
    svissr_r6_t X_COMP_EARTH_FIXED_SPEED; /* R*6.10, X component of the satellite position in earth-fixed coordinates, [m/s] */
    svissr_r6_t Y_COMP_EARTH_FIXED_SPEED; /* R*6.10, Y component of the satellite position in earth-fixed coordinates, [m/s] */
    svissr_r6_t Z_COMP_EARTH_FIXED_SPEED; /* R*6.10, Z component of the satellite position in earth-fixed coordinates, [m/s] */
    svissr_r6_t GREENWHICH_SIDEREAL;      /* R*6.8, Greenwhich sidereal time in true of data coordinates, [deg] */
    svissr_r6_t RATS_J2;                  /* R*6.8, Right ascension from the satellite to the sun in mean of J2000 coordinates, [deg] */
    svissr_r6_t DSTS_J2;                  /* R*6.8, Declination from the satellite to the sun in mean of J2000 coordinates, [deg] */
    svissr_r6_t RATS_EF;                  /* R*6.8, Right ascension from the satellite to the sun in earth-fixed coordinates, [deg] */
    svissr_r6_t DSTS_EF;                  /* R*6.8, Declination from the satellite to the sun in earth-fixed coordinates, [deg] */
    uint8_t spare[14];
    svissr_r6_t NPM_R1_C1;     /* R6*12 Nutation and precession matrix row 1 column 1 */
    svissr_r6_t NPM_R2_C1;     /* R6*14 Nutation and precession matrix row 2 column 1 */
    svissr_r6_t NPM_R3_C1;     /* R6*14 Nutation and precession matrix row 3 column 1 */
    svissr_r6_t NPM_R1_C2;     /* R6*14 Nutation and precession matrix row 1 column 2 */
    svissr_r6_t NPM_R2_C2;     /* R6*12 Nutation and precession matrix row 2 column 2 */
    svissr_r6_t NPM_R3_C2;     /* R6*16 Nutation and precession matrix row 3 column 2 */
    svissr_r6_t NPM_R1_C3;     /* R6*12 Nutation and precession matrix row 1 column 3 */
    svissr_r6_t NPM_R2_C3;     /* R6*16 Nutation and precession matrix row 2 column 3 */
    svissr_r6_t NPM_R3_C3;     /* R6*12 Nutation and precession matrix row 3 column 3 */
    svissr_r6_t SSP_LATITUDE;  /* R*6.8, North Latitude, [deg] */
    svissr_r6_t SSP_LONGITUDE; /* R*6.8, East Longitude, [deg] */
    svissr_r6_t ALTITUDE;      /* R*6.6, Height of sat above Earth surface, [m] */
    uint8_t spare2[56];
}

#ifdef _WIN32
;
#else
__attribute__((packed));
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif

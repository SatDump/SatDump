#include "common/big_endian.h"
#include <cstdint>

#ifdef _WIN32
#pragma pack(push, 1)
#endif

struct AttitudePredictionData
{
    be_uint48_t prediction_time;           /* R*6.8, UTC time when attitude prediction is processed, [MJD] */
    be_uint48_t anno_domini_bcd;           /* BCD*6, time when attitude prediction is processed, [BCD] */
    be_uint48_t angle_yz_1950;             /* R*6.8, Angle between z-axis and satellite spin axis projected on yz-plane in mean of 1950.0 coordinates, mean J2000, [rad] */
    be_uint48_t spin_axis_yz_angle;        /* R*6.11, Angle between satelltie spin axis and yz-plane, [rad] */
    be_uint48_t dihed_angle;               /* R*6.8, Dihedral angle between the Sun and Earth measured clockwise viewing from North, [rad]*/
    be_uint48_t spin_rate;                 /* R*6.8, Spin speed of satellite, [rpm] */
    be_uint48_t right_ascension_spin_axis; /* R*6.8, Right ascension of satellite spin axis on the sat orbit plane coordinate sysetm, [rad] */
    be_uint48_t declination_spin_axis;     /* R*6.8, Declination of satellite spin axis on the sat orbit plane coordinate sysetm, [rad]*/
    uint8_t spare[16];
};

struct OrbitPredictionData
{

    be_uint48_t prediction_time;                        /* R*6.8, UTC time when orbit prediction is processed, [MJD] */
    be_uint48_t anno_domini_bcd;                        /* BCD*6, time when orbit prediction is processed, [BCD] */
    be_uint48_t x_component;                            /* R*6.8, X component of the satellite position in mean J2000, [m] */
    be_uint48_t y_component;                            /* R*6.8, Y component of the satellite position in mean J2000, [m] */
    be_uint48_t z_component;                            /* R*6.8, Z component of the satellite position in mean J2000, [m] */
    be_uint48_t x_component_speed;                      /* R*6.8, X component of the satellite position in mean J2000, [m/s] */
    be_uint48_t y_component_speed;                      /* R*6.8, Y component of the satellite position in mean J2000, [m/s] */
    be_uint48_t z_component_speed;                      /* R*6.8, Z component of the satellite position in mean J2000, [m/s] */
    be_uint48_t x_component_earth_fixed;                /* R*6.6, X component of the satellite position in earth-fixed coordinates, [m] */
    be_uint48_t y_component_earth_fixed;                /* R*6.6, Y component of the satellite position in earth-fixed coordinates, [m] */
    be_uint48_t z_component_earth_fixed;                /* R*6.6, Z component of the satellite position in earth-fixed coordinates, [m] */
    be_uint48_t x_component_earth_fixed_speed;          /* R*6.10, X component of the satellite position in earth-fixed coordinates, [m/s] */
    be_uint48_t y_component_earth_fixed_speed;          /* R*6.10, Y component of the satellite position in earth-fixed coordinates, [m/s] */
    be_uint48_t z_component_earth_fixed_speed;          /* R*6.10, Z component of the satellite position in earth-fixed coordinates, [m/s] */
    be_uint48_t greenwhich_sidereal;                    /* R*6.8, Greenwhich sidereal time in true of data coordinates, [deg] */
    be_uint48_t right_ascension_sat_to_sun_j2000;       /* R*6.8, Right ascension from the satellite to the sun in mean of J2000 coordinates, [deg] */
    be_uint48_t declination_sat_to_sun_j2000;           /* R*6.8, Declination from the satellite to the sun in mean of J2000 coordinates, [deg] */
    be_uint48_t right_ascension_sat_to_sun_earth_fixed; /* R*6.8, Right ascension from the satellite to the sun in earth-fixed coordinates, [deg] */
    be_uint48_t declination_sat_to_sun_earth_fixed;     /* R*6.8, Declination from the satellite to the sun in earth-fixed coordinates, [deg] */
    uint8_t spare[14];
    be_uint48_t nutation_and_precession_matrix_row1_column1; /* R6*12 */
    be_uint48_t nutation_and_precession_matrix_row2_column1; /* R6*14 */
    be_uint48_t nutation_and_precession_matrix_row3_column1; /* R6*14 */
    be_uint48_t nutation_and_precession_matrix_row1_column2; /* R6*14 */
    be_uint48_t nutation_and_precession_matrix_row2_column2; /* R6*12 */
    be_uint48_t nutation_and_precession_matrix_row3_column2; /* R6*16 */
    be_uint48_t nutation_and_precession_matrix_row1_column3; /* R6*12 */
    be_uint48_t nutation_and_precession_matrix_row2_column3; /* R6*16 */
    be_uint48_t nutation_and_precession_matrix_row3_column3; /* R6*12 */
    be_uint48_t sub_sat_point_latitude;                      /* R*6.8, North Latitude ,[deg] */
    be_uint48_t sub_sat_point_longitude;                     /* R*6.8, Eart Longitude ,[deg] */
    be_uint48_t height_above_earth_surface;                  /* R*6.6, Height of sat above Earth surface, [m] */
    uint8_t spare2[56];
};

struct OrbitAndAttitudeData
{
    be_uint48_t observation_start_time;                  /* R*6.8 */
    be_uint32_t VIS_stepping_angle_along_line;           /* R*4.8 [rad] */
    be_uint32_t IR_stepping_angle_along_line;            /* R*4.8 [rad] */
    be_uint32_t VIS_stepping_angle_along_pixel;          /* R*4.10 [rad] */
    be_uint32_t IR_stepping_angle_along_pixel;           /* R*4.10 [rad] */
    be_uint32_t VIS_channel_center_line_number_of_frame; /* R*4.4 */
    be_uint32_t IR_channel_center_line_number_of_frame;  /* R*4.4 */
    be_uint32_t VIS_channel_center_pixel_number;         /* R*4.4 */
    be_uint32_t IR_channel_center_pixel_number;          /* R*4.4 */
    be_uint32_t VIS_channel_sensor_number;               /* R*4.0 */
    be_uint32_t IR_channel_sensor_number;                /* R*4.0 */
    be_uint32_t VIS_total_line_number_of_frame;          /* R*4.0 */
    be_uint32_t IR_total_line_number_of_frame;           /* R*4.0 */
    be_uint32_t VIS_pixels_per_line;                     /* R*4.0 */
    be_uint32_t IR_pixels_per_line;                      /* R*4.0 */
    be_uint32_t VISSR_misalignment_angle_x_axis;         /* R*4.10 [rad] */
    be_uint32_t VISSR_misalignment_angle_y_axis;         /* R*4.10 [rad] */
    be_uint32_t VISSR_misalignment_angle_z_axis;         /* R*4.10 [rad] */

    // second page
    be_uint32_t VISSR_misalignment_matrix_row1_column1;  /* R*4.7, fixed value */
    be_uint32_t VISSR_misalignment_matrix_row2_column1;  /* R*4.10, fixed value */
    be_uint32_t VISSR_misalignment_matrix_row3_column1;  /* R*4.10, fixed value */
    be_uint32_t VISSR_misalignment_matrix_row1_column2;  /* R*4.10, fixed value */
    be_uint32_t VISSR_misalignment_matrix_row2_column2;  /* R*4.7, fixed value */
    be_uint32_t VISSR_misalignment_matrix_row3_column2;  /* R*4.10, fixed value */
    be_uint32_t VISSR_misalignment_matrix_row1_column3;  /* R*4.10, fixed value */
    be_uint32_t VISSR_misalignment_matrix_row2_column3;  /* R*4.10, fixed value */
    be_uint32_t VISSR_misalignment_matrix_row3_column3;  /* R*4.7, fixed value */
    be_uint32_t IR2_channel_center_line_number_of_frame; /* R*4.4 */
    be_uint32_t IR3_channel_center_line_number_of_frame; /* R*4.4 */
    uint32_t junk_1;
    uint32_t junk_2;
    uint32_t spare;
    be_uint32_t constants_ratio_of_circumference;           /* R*4.7, fixed value, ratio of circumference pi */
    be_uint32_t ratio_of_circumference_180;                 /* R*4.9, fixed value, pi/180 */
    be_uint32_t _180_ratio_of_circumference;                /* R*4.9, fixed value, 180/pi */
    be_uint32_t equatorial_radius_of_Earth;                 /* R*4.1, fixed value, [m] */
    be_uint32_t oblateness_of_Earth;                        /* R*4.10, fixed value */
    be_uint32_t eccentricity_of_Earth;                      /* R*4.9, fixed value*/
    be_uint32_t angle_between_VISSR_and_view_of_sun_sensor; /* R*4.8, fixed value, Angle of data acquisition start, [rad]*/

    // third page
    be_uint48_t epoch_time_of_orbital_parameters;                          /* R*6.8, Orbit J2000, [MJD] */
    be_uint48_t semi_major_axis;                                           /* R*6.8, Orbit J2000, [km] */
    be_uint48_t eccentricity;                                              /* R*6.10, Orbit J2000 */
    be_uint48_t inclination;                                               /* R*6.8, Orbit J2000, [deg] */
    be_uint48_t longitude_of_ascending_node;                               /* R*6.8, Orbit J2000, [deg] */
    be_uint48_t argument_of_perigee;                                       /* R*6.8, Orbit J2000, [deg] */
    be_uint48_t mean_anomaly;                                              /* R*6.8, Orbit J2000, [deg] */
    be_uint48_t sub_satellite_east_longitude;                              /* R*6.6, Orbit J2000, [deg] */
    be_uint48_t sub_satellite_north_latitude;                              /* R*6.6, Orbit J2000, [deg] */
    be_uint48_t epoch_time_of_attitude_parameters;                         /* R*6.8, Orbit J2000, [MJD] */
    be_uint48_t angle_between_z_axis_and_satellite_spin_on_yz_plane_alpha; /* R*6.8, Attitude J2000, [rad] */
    be_uint48_t change_rate_of_alpha;                                      /* R*6.15, Attitude J2000, [rad/sec] */
    be_uint48_t angle_between_satellite_spin_axis_and_yz_plane_delta;      /* R*6.11, Attitude J2000, [rad] */
    be_uint48_t change_rate_of_delta;                                      /* R*6.15, Attitude J2000, [rad/sec] */
    be_uint48_t daily_mean_satellite_spin_Rate;                            /* R*6.8, Estimated spin rate, [rpm] */
    uint8_t spare2[10];

    // fourth page
    struct AttitudePredictionData attitude_subblocks[10];
    struct OrbitPredictionData orbit_prediction_subblocks[8];
    be_uint48_t first_attitude_prediciton;    /* R*6.8, Time of the first attitude prediction record, [MJD]*/
    be_uint48_t last_attitude_prediciton;     /* R*6.8, Time of the last attitude prediction record, [MJD]*/
    be_uint48_t interval_attitude_prediction; /* R*6.8, fixed value (0.00347222), Interval time of attitude prediction data, [MJD]*/
    uint32_t number_attitude_prediction;      /* I*2 Number of attitude prediction data block, [=10] */
    be_uint48_t first_orbit_prediciton;       /* R*6.8, Time of the first orbit prediction record, [MJD]*/
    be_uint48_t last_orbit_prediciton;        /* R*6.8, Time of the last orbit prediction record, [MJD]*/
    be_uint48_t interval_orbit_prediction;    /* R*6.8, fixed value (0.00347222), Interval time of orbit prediction data, [MJD]*/
    uint32_t number_orbit_prediction;         /* I*2 Number of attitude prediction data block, [=8] */
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
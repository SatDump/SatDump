#pragma once

#include <string>

/**
 * Speed of light in m/s
 */
#define LIGHT_SPEED 299792458

/**
 * Boltzman's constant dBW/KHz
 */
#define BOLTZMANS_CONST -228.6

/**
 * Ground station effective noise temperature in K
 */
#define GS_NOISE_TEMP 510

/*!
 * A struct that contains information about the acquisition of signal (AOS),
 * the loss of signal (LOS) and max elevation of a satellite pass.
 */
typedef struct
{
    std::string aos;
    std::string los;
    double max_elevation;
} pass_details_t;

enum impairment_enum_t
{
    IMPAIRMENT_NONE = 0,
    ATMO_GASES_ITU,
    ATMO_GASES_REGRESSION,
    PRECIPITATION_ITU,
    PRECIPITATION_CUSTOM,
    FREE_SPACE_PATH_LOSS,
    ANTENNA_POINTING_LOSS,
    DOPPLER_SHIFT
};

enum noise_t
{
    NOISE_NONE = 0,
    WHITE_GAUSSIAN
};

enum polarization_t
{
    RHCP = 0,
    LHCP,
    LINEAR_VERTICAL,
    LINEAR_HORIZONTAL
};

/**
 * Radius of the Earth in km
 */
#define EARTH_RADIUS 6356.766
#define MATH_PI 3.14159265358979323846

#include "parabolic_reflector.h"
#include "common/geodetic/euler_coordinates.h"
#include "logger.h"
#include <cmath>

namespace antenna
{
    ParabolicReflectorAntenna::ParabolicReflectorAntenna(uint8_t type, double frequency, int polarization, double pointing_error, double diameter, double aperture_efficiency)
        : GenericAntenna(PARABOLIC_REFLECTOR, frequency, polarization, pointing_error), d_diameter(diameter), d_aperture_efficiency(aperture_efficiency)

    {
        logger->trace("Parabolic Reflector");
        logger->trace("Maximum Gain: %f", get_gain());
        logger->trace("Beamwidth: %f", get_beamwidth());
    }

    ParabolicReflectorAntenna::~ParabolicReflectorAntenna() {}

    double ParabolicReflectorAntenna::get_gain() { return 20.4 + 20 * std::log10(d_diameter) + 20 * std::log10(d_frequency / 1e6 / 1000) + 10 * std::log10(d_aperture_efficiency / 100); }

    double ParabolicReflectorAntenna::get_gain_rolloff()
    {
        double error_deg = d_pointing_error * RAD_TO_DEG;
        double tmp = 2 * (error_deg * (79.76 / get_beamwidth()));
        if (!error_deg)
        {
            return 0;
        }
        else
        {
            return -10 * std::log10(3282.1 * (std::pow(std::sin(tmp * DEG_TO_RAD), 2) / std::pow(tmp, 2)));
        }
    }

    double ParabolicReflectorAntenna::get_beamwidth() { return 21 / (d_diameter * (d_frequency / 1e6 / 1000)); }
} // namespace antenna
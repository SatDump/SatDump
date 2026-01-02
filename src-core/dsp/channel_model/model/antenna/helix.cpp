#include "helix.h"
#include "common/geodetic/geodetic_coordinates.h"
#include "generic.h"
#include "logger.h"
#include <cmath>

namespace antenna
{
    HelixAntenna::HelixAntenna(uint8_t type, double frequency, int polarization, double pointing_error, size_t turns, double turn_spacing, double circumference)
        : GenericAntenna(HELIX, frequency, polarization, pointing_error), d_turns(turns), d_turn_spacing(turn_spacing), d_circumference(circumference)
    {
        logger->trace("HELIX");
        logger->trace("Maximum Gain: %f", get_gain());
        logger->trace("Beamwidth: %f", get_beamwidth());
    }

    HelixAntenna::~HelixAntenna() {}

    double HelixAntenna::get_gain() { return 10 * std::log10(15 * std::pow(d_circumference, 2) * d_turns * d_turn_spacing); }

    double HelixAntenna::get_gain_rolloff()
    {
        double error_deg = d_pointing_error * RAD_TO_DEG;
        double tmp = 2 * error_deg * (79.76 / get_beamwidth());
        if (error_deg > 0)
        {
            return -10 * std::log10(3282.81 * std::pow(std::sin(tmp * DEG_TO_RAD), 2) / std::pow(tmp, 2));
        }
        else
        {
            return 0;
        }
    }

    double HelixAntenna::get_beamwidth() { return 115 / (d_circumference * std::sqrt(d_turns * d_turn_spacing)); }
} // namespace antenna
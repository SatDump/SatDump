#include "dipole.h"
#include "common/geodetic/geodetic_coordinates.h"
#include "generic.h"
#include "logger.h"
#include <cmath>

namespace antenna
{
    DipoleAntenna::DipoleAntenna(uint8_t type, double frequency, int polarization, double pointing_error) : GenericAntenna(DIPOLE, frequency, polarization, pointing_error)
    {
        logger->trace("DIPOLE");
        logger->trace("Maximum Gain: %f", get_gain());
        logger->trace("Beamwidth: %f", get_beamwidth());
    }

    DipoleAntenna::~DipoleAntenna() {}

    double DipoleAntenna::get_gain() { return 2.15; }

    double DipoleAntenna::get_gain_rolloff()
    {
        double error_deg = d_pointing_error * RAD_TO_DEG;
        if (error_deg < 90)
        {
            if (!error_deg)
            {
                return 0;
            }
            return -10 * std::log10(std::cos(error_deg * DEG_TO_RAD));
        }
        else
        {
            return -10 * log10(-std::cos(error_deg * DEG_TO_RAD));
        }
    }

    double DipoleAntenna::get_beamwidth() { return 156.2; }

} // namespace antenna
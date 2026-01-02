#include "quadrafilar_helix.h"
#include "common/geodetic/geodetic_coordinates.h"
#include "generic.h"
#include "logger.h"
#include <cmath>

namespace antenna
{
    QuadrifilarHelixAntenna::QuadrifilarHelixAntenna(uint8_t type, double frequency, int polarization, double pointing_error, double loop)
        : GenericAntenna(QUADRIFILAR_HELIX, frequency, polarization, pointing_error), d_loop(loop)
    {
        logger->trace("QUADRIFILAR HELIX");
        logger->trace("Maximum Gain: %f", get_gain());
        logger->trace("Beamwidth: %f", get_beamwidth());
    }

    QuadrifilarHelixAntenna::~QuadrifilarHelixAntenna() {}

    double QuadrifilarHelixAntenna::get_gain() { return 4; }

    double QuadrifilarHelixAntenna::get_gain_rolloff()
    {
        double error_deg = d_pointing_error * RAD_TO_DEG;
        return -1.5 * (-4 + 10 * std::log10(1.256 * (1 + std::cos(d_pointing_error))));
    }

    double QuadrifilarHelixAntenna::get_beamwidth() { return 150; }
} // namespace antenna
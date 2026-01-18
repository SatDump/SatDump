#include "generic.h"
#include "../const.h"
#include "common/geodetic/geodetic_coordinates.h"

namespace antenna
{
    GenericAntenna::GenericAntenna(uint8_t type, double frequency, int polarization, double pointing_error)
        : d_type(type), d_frequency(frequency), d_polarization(polarization), d_pointing_error(pointing_error * DEG_TO_RAD)
    {
        my_id = base_unique_id++;
    }

    GenericAntenna::~GenericAntenna() {}

    double GenericAntenna::get_frequency() { return d_frequency; }

    double GenericAntenna::get_pointing_error() { return d_pointing_error; }

    void GenericAntenna::set_pointing_error(double error) { d_pointing_error = error * DEG_TO_RAD; }

    double GenericAntenna::get_wavelength() { return LIGHT_SPEED / d_frequency; }

    int GenericAntenna::get_polarization()
    {
        // FIXME
        return d_polarization;
    }

    int GenericAntenna::base_unique_id = 1;

    int GenericAntenna::unique_id() { return my_id; }
} // namespace antenna
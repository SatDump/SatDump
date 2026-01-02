#include "generic.h"

GenericAttenuation::GenericAttenuation() {}

GenericAttenuation::~GenericAttenuation() {}

void GenericAttenuation::set_frequency(double freq) { frequency = freq / 1e9; }

void GenericAttenuation::set_slant_range(double range) { slant_range = range; }

void GenericAttenuation::set_polarization(uint8_t polar) { polarization = polar; }

void GenericAttenuation::set_elevation_angle(double elev_angle) { elevation_angle = elev_angle; }
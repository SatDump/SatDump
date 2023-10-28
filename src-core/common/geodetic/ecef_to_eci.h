#pragma once

/*
Convert GPS ECEF ephemeris to ECI-based
ephemeris alike SGP4.

Input it in m & m/s, output is in km & km/s
*/
void ecef_epehem_to_eci(double time, double &ephem_x, double &ephem_y, double &ephem_z, double &ephem_vx, double &ephem_vy, double &ephem_vz);

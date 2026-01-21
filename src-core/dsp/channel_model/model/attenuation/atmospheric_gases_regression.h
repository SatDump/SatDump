#pragma once

#include "generic.h"
#include <vector>
#include <cstddef>

class AtmosphericGasesRegression : public GenericAttenuation
{
public:
    AtmosphericGasesRegression(double surface_watervap_density, double temperature);

    ~AtmosphericGasesRegression();

    double get_attenuation();

private:
    double d_surface_watervap_density;
    double d_temperature;
    double d_af, d_bf, d_cf;
    double d_azf, d_bzf, d_czf;

    typedef std::tuple<double, double, double, double> atmo_coefficients_t;

    /*!
     * \brief Data taken from Ippolito - Radiowave Propagation in Satellite Communications
     * Table 3-1  Coefficients for the Calculation of specific attenuation due
     * to gaseous absorption.
     */
    std::vector<atmo_coefficients_t> d_atmo_gases_coeff{atmo_coefficients_t(1, 0.00588, 0.0000178, 0.0000517), atmo_coefficients_t(4, 0.00802, 0.000141, 0.0000850),
                                                        atmo_coefficients_t(6, 0.00824, 0.000300, 0.0000895), atmo_coefficients_t(12, 0.00898, 0.00137, 0.000108)};

    /*!
     * \brief Data taken from Ippolito - Radiowave Propagation in Satellite Communications
     * Table 3-2  Coefficients for the Calculation of Total Zenith Atmospheric Attenuation.
     */
    std::vector<atmo_coefficients_t> d_atmo_gases_coeff_zenith{
        atmo_coefficients_t(1, 0.0334, 0.00000276, 0.000112),
        atmo_coefficients_t(4, 0.0397, 0.000276, 0.000176),
        atmo_coefficients_t(6, 0.0404, 0.000651, 0.000196),
        atmo_coefficients_t(12, 0.0436, 0.00318, 0.000315),
    };

    double m(double y1, double y2, double f1, double f2);

    double calc_coeff(double y1, double y2, double f1, double f2, double f0);

    AtmosphericGasesRegression::atmo_coefficients_t get_atmo_coeff(double frequency, std::vector<atmo_coefficients_t> *coeff_table);
};
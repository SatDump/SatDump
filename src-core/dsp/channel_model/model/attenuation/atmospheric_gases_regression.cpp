#include "atmospheric_gases_regression.h"
#include "../const.h"
#include "common/geodetic/euler_coordinates.h"
#include "generic.h"
#include <cmath>
#include <stdexcept>

AtmosphericGasesRegression::AtmosphericGasesRegression(double watervap, double temperature) : GenericAttenuation(), d_surface_watervap_density(watervap), d_temperature(temperature)

{
    AtmosphericGasesRegression::atmo_coefficients_t tmp_coeff;
    tmp_coeff = get_atmo_coeff(frequency, &d_atmo_gases_coeff);

    d_af = std::get<1>(tmp_coeff);
    d_bf = std::get<2>(tmp_coeff);
    d_cf = std::get<3>(tmp_coeff);

    tmp_coeff = get_atmo_coeff(frequency, &d_atmo_gases_coeff_zenith);

    d_azf = std::get<1>(tmp_coeff);
    d_bzf = std::get<2>(tmp_coeff);
    d_czf = std::get<3>(tmp_coeff);
}

AtmosphericGasesRegression::~AtmosphericGasesRegression() {}

double AtmosphericGasesRegression::m(double y1, double y2, double f1, double f2) { return std::log10(y1 / y2) / std::log10(f1 / f2); }

double AtmosphericGasesRegression::calc_coeff(double y1, double y2, double f1, double f2, double f0)
{
    double _m = m(y1, y2, f1, f2);
    return std::pow(10, _m * std::log10(f0) + (std::log10(y2) - _m * std::log10(f2)));
}

AtmosphericGasesRegression::atmo_coefficients_t AtmosphericGasesRegression::get_atmo_coeff(double frequency, std::vector<atmo_coefficients_t> *coeff_table)
{
    double a_f, b_f, c_f = 0;

    if (frequency < std::get<0>((*coeff_table)[0]))
    {
        a_f = std::get<1>((*coeff_table)[0]);
        b_f = std::get<2>((*coeff_table)[0]);
        c_f = std::get<3>((*coeff_table)[0]);
    }
    else if (frequency > std::get<0>((*coeff_table)[coeff_table->size() - 1]))
    {
        a_f = std::get<1>((*coeff_table)[0]);
        b_f = std::get<2>((*coeff_table)[0]);
        c_f = std::get<3>((*coeff_table)[0]);
    }
    else
    {
        for (size_t i = 0; i < coeff_table->size(); i++)
        {
            if (std::get<0>((*coeff_table)[i]) == frequency)
            {
                a_f = std::get<1>((*coeff_table)[i]);
                b_f = std::get<2>((*coeff_table)[i]);
                c_f = std::get<3>((*coeff_table)[i]);
                break;
            }
            else if (std::get<0>((*coeff_table)[i]) > frequency)
            {
                a_f = calc_coeff(std::get<1>((*coeff_table)[i - 1]), std::get<1>((*coeff_table)[i]), std::get<0>((*coeff_table)[i - 1]), std::get<0>((*coeff_table)[i]), frequency);
                b_f = calc_coeff(std::get<2>((*coeff_table)[i - 1]), std::get<2>((*coeff_table)[i]), std::get<0>((*coeff_table)[i - 1]), std::get<0>((*coeff_table)[i]), frequency);
                c_f = calc_coeff(std::get<3>((*coeff_table)[i - 1]), std::get<3>((*coeff_table)[i]), std::get<0>((*coeff_table)[i - 1]), std::get<0>((*coeff_table)[i]), frequency);
                break;
            }
        }
    }

    return atmo_coefficients_t(frequency, a_f, b_f, c_f);
}

double AtmosphericGasesRegression::get_attenuation()
{
    double gammaa = d_af + d_bf * d_surface_watervap_density - d_cf * d_temperature;
    double zenitha = d_azf + d_bzf * d_surface_watervap_density - d_czf * d_temperature;

    double ha = zenitha / gammaa;

    if (elevation_angle * RAD_TO_DEG >= 10)
    {
        return (ha * zenitha) / std::sin(elevation_angle);
    }
    else
    {
        return (2 * ha * zenitha) / (std::sqrt(std::pow(std::sin(elevation_angle), 2) + ((2 * ha) / EARTH_RADIUS)) + std::sin(elevation_angle));
    }
}
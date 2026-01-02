#include "atmospheric_gases_itu.h"
#include "../const.h"
#include "generic.h"
#include <cmath>
#include <stdexcept>

AtmosphericGasesITU::AtmosphericGasesITU(double surface_watervap_density)
    : GenericAttenuation(), d_oxygen_pressure(0), d_temperature(0), d_water_pressure(0), d_surface_watervap_density(surface_watervap_density)
{
}

AtmosphericGasesITU::~AtmosphericGasesITU() {}

double AtmosphericGasesITU::geopotential_to_geometric(double alt) { return (6356.766 * alt) / (6356.766 - alt); }

double AtmosphericGasesITU::geometric_to_geopotential(double alt) { return (6356.766 * alt) / (6356.766 + alt); }

double AtmosphericGasesITU::get_temperature(double alt)
{
    double geopotential_alt = geometric_to_geopotential(alt);
    double temperature = 0;
    /**
     * Troposphere
     */
    if (geopotential_alt >= 0 && geopotential_alt <= 11)
    {
        temperature = 288.15 - 6.5 * geopotential_alt;
    }
    /**
     * Tropopause
     */
    else if (geopotential_alt > 11 && geopotential_alt <= 20)
    {
        temperature = 216.65;
    }
    /**
     * Stratosphere
     */
    else if (geopotential_alt > 20 && geopotential_alt <= 32)
    {
        temperature = 216.65 + geopotential_alt - 20;
    }
    /**
     * Stratosphere
     */
    else if (geopotential_alt > 32 && geopotential_alt <= 47)
    {
        temperature = 228.65 + 2.8 * (geopotential_alt - 32);
    }
    /**
     * Stratopause
     */
    else if (geopotential_alt > 47 && geopotential_alt <= 51)
    {
        temperature = 270.65;
    }
    /**
     * Mesosphere
     */
    else if (geopotential_alt > 51 && geopotential_alt <= 71)
    {
        temperature = 270.65 - 2.8 * (geopotential_alt - 51);
    }
    /**
     * Mesosphere
     */
    else if (geopotential_alt > 71 && geopotential_alt <= 84.852)
    {
        temperature = 270.65 - 2.8 * (geopotential_alt - 51);
    }
    /**
     * Mesopause
     */
    else if (geopotential_alt > 86 && geopotential_alt <= 91)
    {
        temperature = 186.8673;
    }
    else if (alt > 91)
    {
        temperature = 263.1905 - 76.3232 * std::pow(1 - std::pow((alt - 91) / 19.9429, 2), 0.5);
    }
    return temperature;
}

double AtmosphericGasesITU::get_pressure(double alt)
{
    double geopotential_alt = geometric_to_geopotential(alt);
    double pressure = 0;
    const double a0 = 95.571899;
    const double a1 = -4.011801;
    const double a2 = 6.424731e-2;
    const double a3 = -4.789660e-4;
    const double a4 = 1.340543e-6;

    /**
     * Troposphere
     */
    if (geopotential_alt >= 0 && geopotential_alt <= 11)
    {
        pressure = 1013.25 * std::pow(288.15 / (288.15 - 6.5 * geopotential_alt), -34.1632 / 6.5);
    }
    /**
     * Tropopause
     */
    else if (geopotential_alt > 11 && geopotential_alt <= 20)
    {
        pressure = 226.3226 * std::exp(-34.1632 * (geopotential_alt - 11) / 216.65);
    }
    /**
     * Stratosphere
     */
    else if (geopotential_alt > 20 && geopotential_alt <= 32)
    {
        pressure = 54.74980 * std::pow(216.65 / (216.65 + geopotential_alt - 20), -34.1632);
    }
    /**
     * Stratosphere
     */
    else if (geopotential_alt > 32 && geopotential_alt <= 47)
    {
        pressure = 8.680422 * std::pow(228.65 / (228.65 + 2.8 * (geopotential_alt - 32)), -34.1632 / 2.8);
    }
    /**
     * Stratopause
     */
    else if (geopotential_alt > 47 && geopotential_alt <= 51)
    {
        pressure = 1.109106 * std::exp(-34.1632 * (geopotential_alt - 47) / 270.65);
    }
    /**
     * Mesosphere
     */
    else if (geopotential_alt > 51 && geopotential_alt <= 71)
    {
        pressure = 0.6694167 * std::pow(270.65 / (270.65 - 2.8 * (geopotential_alt - 51)), -34.1632 / 2.8);
    }
    /**
     * Mesosphere
     */
    else if (geopotential_alt > 71 && geopotential_alt <= 84.852)
    {
        pressure = 0.03956649 * std::pow(214.65 / (214.65 - 2 * (geopotential_alt - 71)), -34.1632 / 2);
    }
    /**
     * Mesopause
     */
    else if (geopotential_alt > 86)
    {
        pressure = std::exp(a0 + a1 * geopotential_alt + a2 * std::pow(geopotential_alt, 2) + a3 * std::pow(geopotential_alt, 3) + a4 * std::pow(geopotential_alt, 4));
    }
    return pressure;
}

double AtmosphericGasesITU::get_water_vapour_pressure(double alt)
{
    double rh = d_surface_watervap_density * std::exp(-alt / 2);
    return (rh * get_temperature(alt)) / 216.7;
}

double AtmosphericGasesITU::S(size_t index, atmo_element_t element)
{
    double theta = 300 / d_temperature;
    switch (element)
    {
    case OXYGEN:
        return d_table1[index][1] * 1e-7 * d_oxygen_pressure * std::pow(theta, 3) * std::exp(d_table1[index][2] * (1 - theta));
        break;
    case WATER_VAPOUR:
        return d_table2[index][1] * (1e-1) * d_water_pressure * std::pow(theta, 3.5) * std::exp(d_table2[index][2] * (1 - theta));
        break;
    default:
        throw std::runtime_error("Invalid atmosphere element!");
    }
}

double AtmosphericGasesITU::F(size_t index, atmo_element_t element)
{
    double theta = 300 / d_temperature;
    double f0;
    double df;
    double delta = 0;
    double result;

    switch (element)
    {
    case OXYGEN:
        f0 = d_table1[index][0];
        df = d_table1[index][3] * (1e-4) * (d_oxygen_pressure * std::pow(theta, 0.8 - d_table1[index][4]) + 1.1 * d_water_pressure * theta);
        // TODO: Equation 6b
        delta = (1e-4) * (d_table1[index][5] + d_table1[index][6] * theta) * (d_oxygen_pressure + d_water_pressure) * std::pow(theta, 0.8);
        result = (frequency / f0) *
                 (((df - delta * (f0 - frequency)) / (std::pow(f0 - frequency, 2) + std::pow(df, 2))) + ((df - delta * (f0 + frequency)) / (std::pow(f0 + frequency, 2) + std::pow(df, 2))));
        break;
    case WATER_VAPOUR:
        f0 = d_table2[index][0];
        df = d_table2[index][3] * 1e-4 * (d_oxygen_pressure * std::pow(theta, d_table1[index][4]) + d_table2[index][5] * d_water_pressure * std::pow(theta, d_table2[index][6]));
        // TODO: Equation 6b
        delta = 0;
        break;
    default:
        throw std::runtime_error("Invalid atmosphere element!");
    }
    result =
        (frequency / f0) * (((df - delta * (f0 - frequency)) / (std::pow(f0 - frequency, 2) + std::pow(df, 2))) + ((df - delta * (f0 + frequency)) / (std::pow(f0 + frequency, 2) + std::pow(df, 2))));

    return result;
}

double AtmosphericGasesITU::ND()
{
    double theta = 300 / d_temperature;
    double d = (d_oxygen_pressure + d_water_pressure) * std::pow(theta, 0.8) * 5.6e-4;
    return frequency * d_oxygen_pressure * std::pow(theta, 2) *
           (6.14e-5 / (d * (1 + std::pow(frequency / d, 2))) + (d_oxygen_pressure * std::pow(theta, 1.5) * 1.4e-12 / (1 + (std::pow(frequency, 1.5) * 1.9e-5))));
}

double AtmosphericGasesITU::N(atmo_element_t element)
{
    double sum = 0;
    switch (element)
    {
    case OXYGEN:
        for (size_t i = 0; i < d_table1.size(); i++)
        {
            sum = sum + S(i, element) * F(i, element);
        }
        return sum + ND();
        break;
    case WATER_VAPOUR:
        for (size_t i = 0; i < d_table2.size(); i++)
        {
            sum = sum + S(i, element) * F(i, element);
        }
        return sum;
        break;
    default:
        throw std::runtime_error("Invalid atmosphere element!");
    }
}

double AtmosphericGasesITU::gamma() { return 0.1820 * frequency * (N(OXYGEN) + N(WATER_VAPOUR)); }

double AtmosphericGasesITU::nh(double temperature, double oxygen_pressure, double watevapour_pressure)
{
    return 1 + (77.6 * (oxygen_pressure / temperature) + 72 * (watevapour_pressure / temperature) + (watevapour_pressure / std::pow(temperature, 2)) * 3.75e5) * 1e-6;
}

double AtmosphericGasesITU::a(double an, double rn, double delta)
{
    double internal = (-std::pow(an, 2) - 2 * rn * delta - std::pow(delta, 2)) / (2 * an * rn + 2 * an * delta);
    if (internal < -1)
    {
        internal = -1;
    }
    else if (internal > 1)
    {
        internal = 1;
    }
    double a_tmp = MATH_PI - acos(internal);
    return a_tmp;
}

double AtmosphericGasesITU::alpha(size_t n, double rn, double delta, double prev_alpha)
{
    double b = beta(n, rn, delta, prev_alpha);
    double alpha_tmp = -rn * cos(b) + 0.5 * std::sqrt(4 * std::pow(rn, 2) * std::pow(cos(b), 2) + (8 * rn * delta) + (4 * std::pow(delta, 2)));
    return alpha_tmp;
}

double AtmosphericGasesITU::beta(size_t n, double rn, double delta, double prev_alpha)
{
    double delta_next = 0.0001 * std::exp(((n + 1) - 1) / 100);
    double aangle;
    double bangle;
    if (n == 1)
    {
        bangle = 1.5707963268 - elevation_angle;
        return bangle;
    }
    else
    {
        aangle = a(prev_alpha, rn, delta);
        bangle = asin((nh(d_temperature, d_oxygen_pressure, d_water_pressure) /
                       nh(get_temperature(rn + delta_next - EARTH_RADIUS), get_pressure(rn + delta_next - EARTH_RADIUS), get_water_vapour_pressure(rn + delta_next - EARTH_RADIUS))) *
                      sin(aangle));
        return bangle;
    }
}

double AtmosphericGasesITU::get_attenuation()
{
    double delta;
    double delta_sum = 0;
    double attenuation_sum = 0;
    double attenuation;
    double rn = EARTH_RADIUS;
    double prev_alpha = 0;
    double alpha_sum = 0;

    /**
     * Method is only valid for elevation angles above 1 degree
     */
    if (elevation_angle < 0.0174533)
    {
        return 0;
    }

    /**
     * Iterate through all atmoshpere layers
     * up to 100km.
     * Each layer has height that exponentially
     * grows from 10cm to 1km
     * TODO: Initial altitude should be related to Ground Station altitude
     */
    for (size_t i = 1; i <= 922; i++)
    {
        delta = 0.0001 * std::exp((i - 1) / 100.0);
        delta_sum += delta;
        rn += delta;
        /**
         * Estimate temperature and pressure for the
         * current atmospheric layer.
         */
        d_temperature = get_temperature(delta_sum);
        d_oxygen_pressure = get_pressure(delta_sum);
        d_water_pressure = get_water_vapour_pressure(delta_sum);
        prev_alpha = alpha(i, rn, delta, prev_alpha);
        /**
         * Ignore some NaN
         * TODO: Investigate this issue
         */
        if (!std::isnan(prev_alpha))
        {
            alpha_sum += prev_alpha;
        }
        attenuation = prev_alpha * gamma();
        if (!std::isnan(attenuation))
        {
            attenuation_sum += attenuation;
        }
    }

    return attenuation_sum;
}

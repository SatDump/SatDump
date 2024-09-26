/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "logger.h"

#include <cmath>
#include "common/calibration.h"

double trapz(std::vector<double> els)
{
    double h = 1;
    double sum = 0;

    for (int i = 0; i < els.size(); i++)
    {
        if (i == 0 || i == els.size() - 1) // for the first and last elements
            sum += els[i] / 2.0;
        else
            sum += els[i]; // the rest of data
    }
    return sum * h; // the result
}

double irrad(double low_wav, double high_wav, double t_bb)
{
    // IRRAD calculate radiant flux of the blackbody over some wavelength interval
    double h = 6.63e-34;  // J-s, Planck's constant
    double k = 1.38e-23;  // J/K, Boltzmann's constant
    double c = 299790000; // m/s, speed of light
    double dwav = (high_wav - low_wav) / 50.0;
    std::vector<double> E_wav;
    double current_wav = low_wav;
    for (int i = 0; i < 50; i++)
    {
        double ewav = temperature_to_radiance(t_bb, freq_to_wavenumber(299792458.0 / current_wav)); // dwav * (2.0 * M_PI * h * pow(c, 2)) / (pow(current_wav, 5) * (exp(h * c / (current_wav * k * t_bb)) - 1.0));
        logger->info(ewav);
        E_wav.push_back(ewav);
        current_wav = low_wav + i * dwav;
    }
    return trapz(E_wav) / (49.0);
}

double calculate_sun_irradiance_interval(double low_wav, double high_wav)
{
    double E_rad_sun = irrad(low_wav, high_wav, 5738);
    logger->info(E_rad_sun);
    double r_eq_sun = 696000000; // m, equatorial radius of sun
    double r_AU = 149597870700;  // m, exact def of 1 AU
    double E_irrad = E_rad_sun * (pow(r_eq_sun, 2) / pow(r_AU, 2));
    return E_irrad;
}

int main(int argc, char *argv[])
{
    initLogger();

    logger->info("%f",  calculate_sun_irradiance_interval(0.56e-6, 0.71e-6));
}
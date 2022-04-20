#pragma once

namespace noaa_metop
{
    namespace mhs
    {
        namespace calibration
        {
            const double wavenumber[5] = {2.968720, 5.236956, 6.114597, 6.114597, 6.348092};
        }
        struct MHS_calibration_Values{
            double f[5][4];
            double RCAL[3];
            double g[5];
            double u_temps[3];
            double u[3][5];
            double ch19_corr[2];
            double wavenumber[5];
            double W[5];
            double Ts[5];
            double To[5];

            //MHS_calibration_Values();
        };
    }
}

#include "modis_calibrator.h"
#include "common/calibration.h"
#include "logger.h"

#include "modis_defs.h"
#include "modis_emiss_table.h"

#include "common/utils.h"

HDF4File Aqua_Coeffs;

namespace eos
{
    namespace modis
    {
        void EosMODISCalibrator::init()
        {
            calculate_rvs_correction();

            for (int scan = 0; scan < (int)d_products->images[7].image.height() / 10; scan++)
            {
                ValsPerScan scaninfo;

                if (!d_vars[scan].contains("mirror_side"))
                    goto skip_scan;
                if (!d_vars[scan].contains("night_group"))
                    goto skip_scan;
                if (!d_vars[scan].contains("space_source"))
                    goto skip_scan;
                if (!d_vars[scan].contains("blackbody_source"))
                    goto skip_scan;
                if (!d_vars[scan].contains("bb_temp"))
                    goto skip_scan;
                if (!d_vars[scan].contains("mir_temp"))
                    goto skip_scan;
                if (!d_vars[scan].contains("cav_temp"))
                    goto skip_scan;
                if (!d_vars[scan].contains("inst_temp"))
                    goto skip_scan;

                scaninfo.MS = d_vars[scan]["mirror_side"];   // Mirror side
                scaninfo.T_bb = get_bb_temperature(scan);    // BB Temperature
                scaninfo.T_mir = get_mir_temperature(scan);  // MIR Temperature
                scaninfo.T_cav = get_cav_temperature(scan);  // CAV Temperature
                scaninfo.T_ins = get_ins_temperature(scan);  // INS Temperature
                get_fp_temperature(scaninfo.fp_temps, scan); // FP Temperatures

                if (scaninfo.T_bb == -1)
                    goto skip_scan;
                if (scaninfo.T_mir == -1)
                    goto skip_scan;
                if (scaninfo.T_cav == -1)
                    goto skip_scan;
                if (scaninfo.T_ins == -1)
                    goto skip_scan;

                for (int D_emiss = 0; D_emiss < 160; D_emiss++)
                {
                    int DN_sv = get_emmissive_view_avg("space_source", D_emiss / 10, D_emiss % 10, scan);     // Space View
                    int DN_bb = get_emmissive_view_avg("blackbody_source", D_emiss / 10, D_emiss % 10, scan); // BlackBody View
                    scaninfo.emissive_DN_SVs[D_emiss] = DN_sv;
                    scaninfo.emissive_DN_BBs[D_emiss] = DN_bb;

                    if (get_emissive_coeffs(scaninfo.emissive_a0[D_emiss],
                                            scaninfo.emissive_a2[D_emiss],
                                            scaninfo.emissive_b1[D_emiss],
                                            scaninfo.emissive_Planck_mir[D_emiss],
                                            DN_sv, DN_bb, scaninfo, D_emiss))
                    {
                        scaninfo.emissive_a0[D_emiss] = -1000;
                        // goto skip_scan;
                    }
                }

                scaninfo.valid = true;
            skip_scan:
                scan_data.push_back(scaninfo);

                logger->warn("Scan %d - MS %d - T_bb %f - T_mir %f - T_cav %f - T_ins %f - FP1 %f - FP2 %f - FP3 %f - FP4 %f - Valid %d - A0 %f - BB %d - SV %d",
                             scan, scaninfo.MS, scaninfo.T_bb, scaninfo.T_mir, scaninfo.T_cav, scaninfo.T_ins,
                             scaninfo.fp_temps[0], scaninfo.fp_temps[1], scaninfo.fp_temps[2], scaninfo.fp_temps[3],
                             (int)scaninfo.valid,
                             scaninfo.emissive_a0[0], scaninfo.emissive_DN_BBs[0], scaninfo.emissive_DN_SVs[0]);
            }
        }

        double EosMODISCalibrator::compute(int channel, int pos_x, int pos_y, int px_val)
        {
            // printf("HEY! %d\n", channel);
            //  250m Reflective Bands
            if (channel < NUM_250M_BANDS)
                return CALIBRATION_INVALID_VALUE;
            // 500m Reflective Bands
            else if (channel < NUM_250M_BANDS + NUM_500M_BANDS)
                return CALIBRATION_INVALID_VALUE;
            // 1000m Reflective Bands
            else if (channel < NUM_250M_BANDS + NUM_500M_BANDS + NUM_1000M_REFL_BANDS - 1)
                return CALIBRATION_INVALID_VALUE;
            // 1000m Emissive Bands
            else if (channel < NUM_250M_BANDS + NUM_500M_BANDS + NUM_1000M_REFL_BANDS + NUM_1000M_EMISS_BANDS)
                return compute_emissive(channel, pos_x, pos_y, px_val);

            return CALIBRATION_INVALID_VALUE;
        }

        inline double spectral_radiance_to_radiance(double L, double wavenumber)
        {
            double c_1 = 1.191042e8;
            double c_2 = 1.4387752e4;
            double lamba = (1e7 / wavenumber) / 1e3;
            double temp = c_2 / (lamba * log(c_1 / (pow(lamba, 5) * L + 1)));
            return temperature_to_radiance(temp, wavenumber);
        }

        double EosMODISCalibrator::compute_emissive(int channel, int pos_x, int pos_y, int px_val)
        {
            int index_channel = channel;
            channel -= NUM_250M_BANDS + NUM_500M_BANDS + NUM_1000M_REFL_BANDS - 1; // Scale channel to "index in category"

            // Skip Band 26
            if (channel == MODIS_BAND26_INDEX_AT_RES)
                return CALIBRATION_INVALID_VALUE;
            if (channel >= MODIS_BAND26_INDEX_AT_RES)
                channel--;

            auto &scaninfo = scan_data[pos_y / 10];

            // Calculate & Check variables we need
            if (!scaninfo.valid)
            {
                printf("Scan Invalid!\n");
                return CALIBRATION_INVALID_VALUE;
            }

            bool MS = scaninfo.MS; // Mirror side

            int D_emiss = channel * 10 + 9 - (pos_y % 10); // Index of emissive detector
            int F = pos_x;                                 // Current frame
            int DN_ev = px_val;                            // Detector value
            int DN_sv = scaninfo.emissive_DN_SVs[D_emiss]; // Space View

            // Check saturation / Missing pixel
            if (DN_ev == 0 || DN_ev == SATURATED_DN)
                return CALIBRATION_INVALID_VALUE;
            if (DN_sv == 0 || DN_sv == SATURATED_DN)
                return CALIBRATION_INVALID_VALUE;

            int dn_ev = DN_ev - DN_sv;

            double a0 = scaninfo.emissive_a0[D_emiss];
            double a2 = scaninfo.emissive_a2[D_emiss];
            double b1 = scaninfo.emissive_b1[D_emiss];
            double L_sm = scaninfo.emissive_Planck_mir[D_emiss];

            if (a0 == -1000)
            {
                printf("Coef Invalid!\n");
                return CALIBRATION_INVALID_VALUE;
            }

            // logger->warn("HEY! %d --- SV %d BB %d EV %d --- BBT %f CAV %f MIR %f INS %f --- dn_bb %d - dn_ev %d - deltaT_bb %f --- a0 %f a2 %f b1 %f -- BBCorr %f SVCorr %f",
            //              D_emiss, DN_sv, DN_bb, DN_ev,
            //              T_bb - 273.15, T_cav - 273.15, T_mir - 273.15, T_ins - 273.15,
            //              dn_bb, dn_ev, T_bb + delta_T_bb - 273.15,
            //              a0, a2, b1,
            //              bb_corr, sv_corr);

            // Compute L_ev
            double Fn = a0 +
                        b1 * dn_ev +
                        a2 * dn_ev * dn_ev;

            // Compute radiance
            double L_ev = (Fn -
                           (RVS_1km_Emiss_SV[D_emiss][MS] - RVS_1km_Emiss_EV[D_emiss][F][MS]) *
                               /*PP_emiss->Planck_mir[D_emiss][S]*/ L_sm) /
                          RVS_1km_Emiss_EV[D_emiss][F][MS];

            return spectral_radiance_to_radiance(L_ev, d_products->get_wavenumber(index_channel));
        }
    }
}
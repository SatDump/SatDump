#include "modis_calibrator.h"
#include "modis_defs.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "core/resources.h"

namespace eos
{
    namespace modis
    {
        namespace precompute
        {

            nlohmann::json precomputeVars(satdump::products::ImageProduct *d_products, nlohmann::json d_vars, bool is_aqua)
            {
                CalibrationVars cvars;

                logger->trace("Loading emissive!");

                nlohmann::json raw_coeffs_emmissive;
                if (is_aqua)
                    raw_coeffs_emmissive = loadCborFile(resources::getResourcePath("calibration/modis_emissive_table_aqua.cbor"));
                else
                    raw_coeffs_emmissive = loadCborFile(resources::getResourcePath("calibration/modis_emissive_table_terra.cbor"));

                Coefficients_Emissive *Sat_CoeffsE = new Coefficients_Emissive(raw_coeffs_emmissive);
                //  logger->trace("Loading reflective!");
                //  Sat_CoeffsR = loadCborFile("/home/alan/Documents/SatDump_ReWork/build/reflective_table_aqua.cbor");

                logger->trace("Calculate RVS/RSB!");
                calculate_rvs_correction(*Sat_CoeffsE, cvars);

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
                    if (!d_vars[scan].contains("fp_temp"))
                        goto skip_scan;
                    if (!d_vars[scan].contains("fp_temp_info"))
                        goto skip_scan;

                    scaninfo.MS = d_vars[scan]["mirror_side"];                    // Mirror side
                    scaninfo.T_bb = get_bb_temperature(d_vars, is_aqua, scan);    // BB Temperature
                    scaninfo.T_mir = get_mir_temperature(d_vars, is_aqua, scan);  // MIR Temperature
                    scaninfo.T_cav = get_cav_temperature(d_vars, is_aqua, scan);  // CAV Temperature
                    scaninfo.T_ins = get_ins_temperature(d_vars, is_aqua, scan);  // INS Temperature
                    get_fp_temperature(d_vars, is_aqua, scaninfo.fp_temps, scan); // FP Temperatures

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
                        int DN_sv = get_emmissive_view_avg(d_vars, "space_source", D_emiss / 10, D_emiss % 10, scan);     // Space View
                        int DN_bb = get_emmissive_view_avg(d_vars, "blackbody_source", D_emiss / 10, D_emiss % 10, scan); // BlackBody View
                        scaninfo.emissive_DN_SVs[D_emiss] = DN_sv;
                        scaninfo.emissive_DN_BBs[D_emiss] = DN_bb;

                        if (get_emissive_coeffs(*Sat_CoeffsE, is_aqua, cvars,
                                                scaninfo.emissive_a0[D_emiss],
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
                    cvars.scan_data.push_back(scaninfo);

                    logger->trace("Scan %d - MS %d - T_bb %f - T_mir %f - T_cav %f - T_ins %f - FP1 %f - FP2 %f - FP3 %f - FP4 %f - Valid %d - A0 %f - BB %d - SV %d",
                                  scan, scaninfo.MS, scaninfo.T_bb, scaninfo.T_mir, scaninfo.T_cav, scaninfo.T_ins,
                                  scaninfo.fp_temps[0], scaninfo.fp_temps[1], scaninfo.fp_temps[2], scaninfo.fp_temps[3],
                                  (int)scaninfo.valid,
                                  scaninfo.emissive_a0[0], scaninfo.emissive_DN_BBs[0], scaninfo.emissive_DN_SVs[0]);
                }

                nlohmann::json finalv;
                finalv["cvars"] = cvars;
                finalv["c_emissive"] = raw_coeffs_emmissive;
                delete Sat_CoeffsE;
                return finalv;
            }

            bool get_emissive_coeffs(Coefficients_Emissive &Sat_CoeffsE, bool is_aqua, CalibrationVars &cvars, double &a0, double &a2, double &b1, float &L_sm, int DN_sv, int DN_bb, ValsPerScan &scani, int D_emiss)
            {
                int MS = scani.MS;

                int dn_bb = DN_bb - DN_sv;

                if (dn_bb < 1.0E-20) // We cannot calibrate this scan
                    return true;

                // bool PCX_correction_switch = Sat_CoeffsE.PCX_correction_switch[0];

                // Calculate a0, a2
                // These are quadratics in instrument temperature (T_ins).
                double T_ins = scani.T_ins;
                a0 = Sat_CoeffsE.A0[0][MS][D_emiss] + T_ins * (Sat_CoeffsE.A0[1][MS][D_emiss] + T_ins * Sat_CoeffsE.A0[2][MS][D_emiss]);
                a2 = Sat_CoeffsE.A2[0][MS][D_emiss] + T_ins * (Sat_CoeffsE.A2[1][MS][D_emiss] + T_ins * Sat_CoeffsE.A2[2][MS][D_emiss]);

                // Calculate dL_bb
                double T_cav = scani.T_cav;
                double T_bb = scani.T_bb;
                double delta_T_bb = Sat_CoeffsE.delta_T_bb_beta[D_emiss] * (T_cav - T_bb) + Sat_CoeffsE.delta_T_bb_delta[D_emiss];

                // Calculate dL_bb
                float T_mir = scani.T_mir;
                if (Calculate_Planck(is_aqua ? Sat_CoeffsE.RSR_AQUA[D_emiss] : Sat_CoeffsE.RSR_TERRA[D_emiss],
                                     is_aqua ? Sat_CoeffsE.WAVELENGTH_AQUA[D_emiss] : Sat_CoeffsE.WAVELENGTH_TERRA[D_emiss],
                                     Sat_CoeffsE.NUM_RSR_vs_Lambda[D_emiss],
                                     T_mir, &L_sm))
                    return true;

                float L_bb = 0;
                if (Calculate_Planck(is_aqua ? Sat_CoeffsE.RSR_AQUA[D_emiss] : Sat_CoeffsE.RSR_TERRA[D_emiss],
                                     is_aqua ? Sat_CoeffsE.WAVELENGTH_AQUA[D_emiss] : Sat_CoeffsE.WAVELENGTH_TERRA[D_emiss],
                                     Sat_CoeffsE.NUM_RSR_vs_Lambda[D_emiss],
                                     T_bb + delta_T_bb, &L_bb))
                    return true;

                float L_cav = 0;
                if (Calculate_Planck(is_aqua ? Sat_CoeffsE.RSR_AQUA[D_emiss] : Sat_CoeffsE.RSR_TERRA[D_emiss],
                                     is_aqua ? Sat_CoeffsE.WAVELENGTH_AQUA[D_emiss] : Sat_CoeffsE.WAVELENGTH_TERRA[D_emiss],
                                     Sat_CoeffsE.NUM_RSR_vs_Lambda[D_emiss],
                                     T_cav, &L_cav))
                    return true;

                float bb_corr = cvars.RVS_1km_Emiss_BB[D_emiss][MS];
                float sv_corr = cvars.RVS_1km_Emiss_SV[D_emiss][MS];

                double dL_bb = bb_corr * Sat_CoeffsE.epsilon_bb[D_emiss] * L_bb +
                               (sv_corr - bb_corr) * L_sm +
                               (1.0 - Sat_CoeffsE.epsilon_bb[D_emiss]) *
                                   Sat_CoeffsE.epsilon_cav[D_emiss] *
                                   bb_corr * L_cav;

                b1 = (dL_bb - a0 - a2 * dn_bb * dn_bb) / dn_bb;

                return false;
            }

            int get_emmissive_view_avg(nlohmann::json &d_vars, std::string type, int channel, int det, int scan)
            {
                float final_avg = 0;
                if (!d_vars[scan].contains(type))
                    return -1;
                // bool night_group = d_vars[scan]["night_group"];

                if (channel >= MODIS_BAND26_INDEX_AT_RES)
                    channel++;

                int valid_cnt = 0;
                for (int i = 0; i < (int)d_vars[scan][type].size(); i++)
                {
                    if (d_vars[scan][type][i].is_null())
                        continue;

                    auto &ifovs = d_vars[scan][type][i];

#if 0
                    if (night_group) // Night
                    {
                        if (ifovs[det * 17 + channel].is_null())
                            continue;

                        float val = ifovs[det * 17 + channel];
                        if (val != 0)
                        {
                            final_avg += val;
                            valid_cnt++;
                        }
                    }
                    else // Day
#endif
                    {
                        if (ifovs[det * 83 + 66 + channel].is_null())
                            continue;

                        float val = ifovs[det * 83 + 66 + channel];
                        if (val != 0)
                        {
                            final_avg += val;
                            valid_cnt++;
                        }
                    }
                }

                return final_avg / valid_cnt;
            }

            void calculate_rvs_correction(Coefficients_Emissive &Sat_CoeffsE, CalibrationVars &cvars)
            {
                bool coeffs_same = 0;
                float rvs_corr = 0, sigma_rvs = 0;
                int frame_no_squared[EV_1km_FRAMES];

                // Initialize the array of squares of frames
                for (int frame = 0; frame < EV_1km_FRAMES; frame++)
                    frame_no_squared[frame] = frame * frame;

                // Frame numbers to use for BB and SV calculations
                int BB_frame_no = Sat_CoeffsE.RVS_BB_SV_Frame_No[0];
                int SV_frame_no = Sat_CoeffsE.RVS_BB_SV_Frame_No[1];

                // Calculate the frame-by-frame RVS Correction Terms for Emissive Bands.
                // Set the emissive detector index to 0.
                int det_160 = 0;

                // Loop through bands, detectors, and mirror sides:
                for (int band = 0; band < NUM_EMISSIVE_BANDS; band++)
                {
                    for (int det = 0; det < DETECTORS_PER_1KM_BAND; det++, det_160++)
                    {
                        for (int mirr_side = 0; mirr_side < NUM_MIRROR_SIDES; mirr_side++)
                        {
                            // Read the set of three rvs coefficients.
                            const float *rvs_coeffs = Sat_CoeffsE.RVS_TEB[band][det][mirr_side];

                            // If this is not the first detector, check to see whether these
                            // coefficients are the same as those for the detector before it.
                            if (det > 0)
                                coeffs_same = (Sat_CoeffsE.RVS_TEB[band][det - 1][mirr_side][0] == rvs_coeffs[0] &&
                                               Sat_CoeffsE.RVS_TEB[band][det - 1][mirr_side][1] == rvs_coeffs[1] &&
                                               Sat_CoeffsE.RVS_TEB[band][det - 1][mirr_side][2] == rvs_coeffs[2]);
                            else
                                coeffs_same = false;

                            bool det_0_or_coeffs_diff = (det == 0 || (coeffs_same == false));

                            // Run through each EV frame and calculate the correction.
                            for (int frame = 0; frame < EV_1km_FRAMES; frame++)
                            {
                                // If this is the first detector or these coefficients are not the
                                // same as those for the previous detector, calculate the new correction.
                                if (det_0_or_coeffs_diff)
                                {
                                    EVAL_2ND_ORDER_POLYNOMIAL(rvs_corr, rvs_coeffs,
                                                              frame, frame_no_squared[frame]);

                                    if (rvs_corr < RVS_CORRECTION_LOWER_LIMIT ||
                                        rvs_corr > RVS_CORRECTION_UPPER_LIMIT)
                                    {
                                        printf("OUT OF RANGE!");
                                        return;
                                    }

                                    // Fill in the calculated correction:
                                    cvars.RVS_1km_Emiss_EV[det_160][frame][mirr_side] = rvs_corr;
                                }
                                else // Coefficients not same
                                     // Otherwise, use the previously recorded correction term.
                                    cvars.RVS_1km_Emiss_EV[det_160][frame][mirr_side] = cvars.RVS_1km_Emiss_EV[det_160 - 1][frame][mirr_side];
                            }

                            // Calculate the sigma RVS for TEB
                            // Read the set of three sigma rvs coefficients.
                            const float *sigma_rvs_coeffs = Sat_CoeffsE.sigma_RVS_EV[band][det][mirr_side];

                            // If this is not the first detector, check to see whether these
                            // coefficients are the same as those for the detector before it.

                            if (det > 0)
                                coeffs_same = (Sat_CoeffsE.sigma_RVS_EV[band][det - 1][mirr_side][0] == sigma_rvs_coeffs[0] &&
                                               Sat_CoeffsE.sigma_RVS_EV[band][det - 1][mirr_side][1] == sigma_rvs_coeffs[1] &&
                                               Sat_CoeffsE.sigma_RVS_EV[band][det - 1][mirr_side][2] == sigma_rvs_coeffs[2]);
                            else
                                coeffs_same = false;

                            det_0_or_coeffs_diff = (det == 0 || (coeffs_same == false));

                            // Run through each EV frame and calculate the correction.
                            for (int frame = 0; frame < EV_1km_FRAMES; frame++)
                            {
                                // If this is the first detector or these coefficients are not the
                                // same as those for the previous detector, calculate the new correction.

                                if (det_0_or_coeffs_diff)
                                {
                                    EVAL_2ND_ORDER_POLYNOMIAL(sigma_rvs, sigma_rvs_coeffs,
                                                              frame, frame_no_squared[frame]);

                                    // Fill in the calculated correction:
                                    cvars.sigma_RVS_Emiss_EV[det_160][frame][mirr_side] = sigma_rvs;

                                } // Coefficients not same
                                else
                                    // Otherwise, use the previously recorded correction term.
                                    cvars.sigma_RVS_Emiss_EV[det_160][frame][mirr_side] = cvars.sigma_RVS_Emiss_EV[det_160 - 1][frame][mirr_side];
                            } // end loop through frame

                            // Fill in the BlackBody correction for this detector and mirror side:
                            cvars.RVS_1km_Emiss_BB[det_160][mirr_side] = cvars.RVS_1km_Emiss_EV[det_160][BB_frame_no][mirr_side];

                            // Fill in the Space View correction for this detector and mirror side:
                            cvars.RVS_1km_Emiss_SV[det_160][mirr_side] = cvars.RVS_1km_Emiss_EV[det_160][SV_frame_no][mirr_side];
                        }
                    }
                }
            }

            bool Calculate_Planck(float *RSR,
                                  float *wl,
                                  int size,
                                  float T,
                                  float *planck)
            /*
            !C************************************************************************
            !Description:  Calculate Planck function integration over RSR

            !Input Parameters:
                float32  *RSR   * RSR values for a particular channel *
                int16    size   * number of valid points for that RSR *
                float32  T      * temperature *
                float32  *wl    * wavelength with same array size*

            !Output Parameters:
                float32 planck  * the planck function integration *

            !Revision History:
             Revision 02.20 March 25, 2002   Razor Issue #178
             Strip out ADC Correction
             Alice Isaacman, SAIC GSO (Alice.R.Isaacman.1@gsfc.nasa.gov)

             Revision 02.10 Mar. 1998
             Initial development
             Shi-Yue Qiu (syqiu@ltpmail.gsfc.nasa.gov)

            !Team-unique Header:
             This software is developed by the MODIS Characterization Support
             Team (MCST)for the National Aeronautics and Space Administration,
             Goddard Space Flight Center, under contract NAS5-32373.

            !References and Credits:
             HDF portions developed at the National Center for Supercomputing
             Applications at the University of Illinois at Urbana-Champaign.

            !Design Notes:

            !END********************************************************************
            */
            {
                // Emissive L_vs_DN Algorithm Planck function MACRO
                // (needs two constants, C_1 and C_2,
                // for radiance in the units of W/m^2/micron/sr)
#define C_1 1.19106e+8
#define C_2 1.43879e+4
#define L(lambda, T) (C_1 / pow((double)(lambda), (double)5) / (exp((double)(C_2 / ((lambda) * (T)))) - 1))

                int i = 0;
                float sum = 0;
                float xL = 0;
                float lambda = 0;
                float dw = 0;

                *planck = 0; /* initialize for error state on return */

                if (size <= 0 || T < 1.0E-20) /* check for bad values */
                    return 1;                 //(returnStatus);

                if (size == 1) /* check for single value */
                {
                    *planck = L(wl[0], T);
                    return 1; //(returnStatus);
                }

                lambda = wl[0];
                dw = 0.5 * (wl[1] - wl[0]);
                xL = RSR[0] * L(lambda, T) * dw;
                sum = RSR[0] * dw;

                for (i = 1; i < size - 1; i++)
                {
                    lambda = wl[i];
                    dw = 0.5 * (wl[i + 1] - wl[i - 1]);
                    xL += RSR[i] * L(lambda, T) * dw;
                    sum += RSR[i] * dw;
                }

                lambda = wl[size - 1];
                dw = 0.5 * (wl[size - 1] - wl[size - 2]);
                xL += RSR[size - 1] * L(lambda, T) * dw;
                sum += RSR[size - 1] * dw;

                if (sum > 0)
                    *planck = xL / sum;

                return 0; // (returnStatus);
            }
        }
    }
}

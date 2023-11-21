#include "modis_calibrator.h"
#include "common/calibration.h"
#include "logger.h"

#include "modis_defs.h"
#include "modis_emiss_table.h"
#include "modis_engcoefs.h"

#include "common/utils.h"

namespace eos
{
    namespace modis
    {
        void EosMODISCalibrator::init()
        {
            calculate_rvs_correction();

            for (int scan = 0; scan < d_products->images[7].image.height() / 10; scan++)
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

                    if (get_coeffs(scaninfo.emissisive_a0[D_emiss],
                                   scaninfo.emissisive_a2[D_emiss],
                                   scaninfo.emissisive_b1[D_emiss],
                                   scaninfo.Planck_mir[D_emiss],
                                   DN_sv, DN_bb, scaninfo, D_emiss))
                    {
                        scaninfo.emissisive_a0[D_emiss] = -1000;
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
                             scaninfo.emissisive_a0[0], scaninfo.emissive_DN_BBs[0], scaninfo.emissive_DN_SVs[0]);
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

        double EosMODISCalibrator::compute_emissive(int channel, int pos_x, int pos_y, int px_val)
        {
            channel -= NUM_250M_BANDS + NUM_500M_BANDS + NUM_1000M_REFL_BANDS - 1; // Scale channel to "index in category"

            // Skip Band 26
            if (channel == MODIS_BAND26_INDEX_AT_RES)
                return CALIBRATION_INVALID_VALUE;
            if (channel >= MODIS_BAND26_INDEX_AT_RES)
                channel--;

            // printf("HEY! %d\n", channel);
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

            double a0 = scaninfo.emissisive_a0[D_emiss];
            double a2 = scaninfo.emissisive_a2[D_emiss];
            double b1 = scaninfo.emissisive_b1[D_emiss];
            double L_sm = scaninfo.Planck_mir[D_emiss];

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

#if 1
            // Compute L_ev
            double Fn = a0 +
                        b1 * dn_ev +
                        a2 * dn_ev * dn_ev;

            // Compute radiance
            double L_ev = (Fn -
                           (RVS_1km_Emiss_SV[D_emiss][MS] - RVS_1km_Emiss_EV[D_emiss][F][MS]) *
                               /*PP_emiss->Planck_mir[D_emiss][S]*/ L_sm) /
                          RVS_1km_Emiss_EV[D_emiss][F][MS];

            return L_ev;
#else
            return dn_ev / 4095.0; // CALIBRATION_INVALID_VALUE;
#endif
        }

        bool EosMODISCalibrator::get_coeffs(double &a0, double &a2, double &b1, float &L_sm, int DN_sv, int DN_bb, ValsPerScan &scani, int D_emiss)
        {
            int MS = scani.MS;

            int dn_bb = DN_bb - DN_sv;

            if (dn_bb < 1.0E-20) // We cannot calibrate this scan
                return true;

            // Calculate a0, a2
            // These are quadratics in instrument temperature (T_ins).
            double T_ins = scani.T_ins;
            a0 = get_A0(0, MS, D_emiss) + T_ins * (get_A0(1, MS, D_emiss) + T_ins * get_A0(2, MS, D_emiss));
            a2 = get_A2(0, MS, D_emiss) + T_ins * (get_A2(1, MS, D_emiss) + T_ins * get_A2(2, MS, D_emiss));

            // Calculate dL_bb
            double T_cav = scani.T_cav;
            double T_bb = scani.T_bb;
            double delta_T_bb = delta_T_bb_beta[D_emiss] * (T_cav - T_bb) + delta_T_bb_delta[D_emiss];

            // Calculate dL_bb
            float T_mir = scani.T_mir;
            if (Calculate_Planck(get_RSR_ptr(D_emiss),
                                 get_WAVELENGTH_ptr(D_emiss),
                                 66, // tables->NUM_RSR_vs_Lambda[D_emiss],
                                 T_mir, &L_sm))
                return true;

            float L_bb = 0;
            if (Calculate_Planck(get_RSR_ptr(D_emiss),
                                 get_WAVELENGTH_ptr(D_emiss),
                                 66, // tables->NUM_RSR_vs_Lambda[D_emiss],
                                 T_bb + delta_T_bb, &L_bb))
                return true;

            float L_cav = 0;
            if (Calculate_Planck(get_RSR_ptr(D_emiss),
                                 get_WAVELENGTH_ptr(D_emiss),
                                 66, // tables->NUM_RSR_vs_Lambda[D_emiss],
                                 T_cav, &L_cav))
                return true;

            float bb_corr = RVS_1km_Emiss_BB[D_emiss][MS];
            float sv_corr = RVS_1km_Emiss_SV[D_emiss][MS];

            double dL_bb = bb_corr * epsilon_bb[D_emiss] * L_bb +
                           (sv_corr - bb_corr) * L_sm +
                           (1.0 - epsilon_bb[D_emiss]) *
                               epsilon_cav[D_emiss] *
                               bb_corr * L_cav;

            b1 = (dL_bb - a0 - a2 * dn_bb * dn_bb) / dn_bb;

            return false;
        }

        int EosMODISCalibrator::get_emmissive_view_avg(std::string type, int channel, int det, int scan)
        {
            float final_avg = 0;
            if (!d_vars[scan].contains(type))
                return -1;
            bool night_group = d_vars[scan]["night_group"];

            if (channel >= MODIS_BAND26_INDEX_AT_RES)
                channel++;

            int valid_cnt = 0;
            for (int i = 0; i < d_vars[scan][type].size(); i++)
            {
                if (d_vars[scan][type][i].is_null())
                    continue;

                auto &ifovs = d_vars[scan][type][i];

                if (night_group) // Night, TODO
                {
                }
                else // Day
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

        void EosMODISCalibrator::calculate_rvs_correction()
        {
            bool coeffs_same = 0;
            float rvs_corr = 0, sigma_rvs = 0;
            int frame_no_squared[EV_1km_FRAMES];

            // Initialize the array of squares of frames
            for (int frame = 0; frame < EV_1km_FRAMES; frame++)
                frame_no_squared[frame] = frame * frame;

            // Frame numbers to use for BB and SV calculations
            int BB_frame_no = RVS_BB_SV_Frame_No[0];
            int SV_frame_no = RVS_BB_SV_Frame_No[1];

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
                        const float *rvs_coeffs = get_RVS_TEB_ptr(band, det, mirr_side);

                        // If this is not the first detector, check to see whether these
                        // coefficients are the same as those for the detector before it.
                        if (det > 0)
                            coeffs_same = (get_RVS_TEB_ptr(band, det - 1, mirr_side)[0] == rvs_coeffs[0] &&
                                           get_RVS_TEB_ptr(band, det - 1, mirr_side)[1] == rvs_coeffs[1] &&
                                           get_RVS_TEB_ptr(band, det - 1, mirr_side)[2] == rvs_coeffs[2]);
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
                                RVS_1km_Emiss_EV[det_160][frame][mirr_side] = rvs_corr;
                            }
                            else // Coefficients not same
                                 // Otherwise, use the previously recorded correction term.
                                RVS_1km_Emiss_EV[det_160][frame][mirr_side] = RVS_1km_Emiss_EV[det_160 - 1][frame][mirr_side];
                        }

                        // Calculate the sigma RVS for TEB
                        // Read the set of three sigma rvs coefficients.
                        const float *sigma_rvs_coeffs = get_sigma_RVS_EV_ptr(band, det, mirr_side);

                        // If this is not the first detector, check to see whether these
                        // coefficients are the same as those for the detector before it.

                        if (det > 0)
                            coeffs_same = (get_sigma_RVS_EV_ptr(band, det - 1, mirr_side)[0] == sigma_rvs_coeffs[0] &&
                                           get_sigma_RVS_EV_ptr(band, det - 1, mirr_side)[1] == sigma_rvs_coeffs[1] &&
                                           get_sigma_RVS_EV_ptr(band, det - 1, mirr_side)[2] == sigma_rvs_coeffs[2]);
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
                                sigma_RVS_Emiss_EV[det_160][frame][mirr_side] = sigma_rvs;

                            } // Coefficients not same
                            else
                                // Otherwise, use the previously recorded correction term.
                                sigma_RVS_Emiss_EV[det_160][frame][mirr_side] = sigma_RVS_Emiss_EV[det_160 - 1][frame][mirr_side];
                        } // end loop through frame

                        // Fill in the BlackBody correction for this detector and mirror side:
                        RVS_1km_Emiss_BB[det_160][mirr_side] = RVS_1km_Emiss_EV[det_160][BB_frame_no][mirr_side];

                        // Fill in the Space View correction for this detector and mirror side:
                        RVS_1km_Emiss_SV[det_160][mirr_side] = RVS_1km_Emiss_EV[det_160][SV_frame_no][mirr_side];
                    }
                }
            }
        }

        double EosMODISCalibrator::get_bb_temperature(int scan)
        {
            Engineering_Coefficients_t Eng_Coeffs = Engineering_Coefficients_FM1;

            double temp = 0, ntemp = 0;
            for (int thermistor = 0; thermistor < NUM_BB_THERMISTORS; thermistor++)
            {
                int DN = d_vars[scan]["bb_temp"][thermistor];

                if (DN > 0 && DN < 4095)
                {
                    double V0 = 0;
                    double I = 0;

                    if (true) // CP_index[S] == ELECTRONICS_PRIMARY)
                    {
                        V0 = Eng_Coeffs.V0_CPA;
                        I = Eng_Coeffs.I_CPA;
                    }
                    else
                    {
                        V0 = Eng_Coeffs.V0_CPB;
                        I = Eng_Coeffs.I_CPB;
                    }

                    double Rc = (5.0 * DN / 4096.0 - (V0 + 2.5)) / I;

                    if (Eng_Coeffs.Rp - Rc == 0)
                    {
                        logger->error("Fail Computing!");
                        continue;
                    }

                    double R = Rc * (Eng_Coeffs.Rp) / (Eng_Coeffs.Rp - Rc);
                    double lnR = log((double)R);

                    if (lnR == 0)
                    {
                        logger->error("Fail Computing!");
                        continue;
                    }

                    double TBB = Eng_Coeffs.BB_a[thermistor][0] + lnR * (Eng_Coeffs.BB_a[thermistor][1] +
                                                                         lnR * (Eng_Coeffs.BB_a[thermistor][2] +
                                                                                lnR * Eng_Coeffs.BB_a[thermistor][3]));
                    double BB_T = 1.0 / TBB;

                    // logger->critical("BBT %d %d %f", thermistor, DN, BB_T - 273.15);

                    temp += BB_T;
                    ntemp++;
                }
            }

            if (ntemp == 0)
                return -1;

            return temp / ntemp;
        }

        double EosMODISCalibrator::get_mir_temperature(int scan)
        {
            Engineering_Coefficients_t Eng_Coeffs = Engineering_Coefficients_FM1;

            double temp = 0, ntemp = 0;
            for (int i = 0; i < 2; i++)
            {
                int MIR = d_vars[scan]["mir_temp"][i];

                if (MIR > 0 && MIR < 4095)
                {
                    float *C_MIR;
                    if (true) // CP_index[S] == ELECTRONICS_PRIMARY)
                        C_MIR = Eng_Coeffs.C_MIR_A;
                    else
                        C_MIR = Eng_Coeffs.C_MIR_B;

                    double mirtemp = 0;
                    CONVERT_TEMP_POLY(mirtemp, MIR, C_MIR, 4095, 273.15);

                    // logger->critical("MIR %d %d %f", i, MIR, mirtemp - 273.15);

                    temp += mirtemp;
                    ntemp++;
                }
            }

            if (ntemp == 0)
                return -1;

            return temp / ntemp;
        }

        double EosMODISCalibrator::get_cav_temperature(int scan)
        {
            Engineering_Coefficients_t Eng_Coeffs = Engineering_Coefficients_FM1;

            double temp = 0, ntemp = 0;
            for (int i = 0; i < 2; i++)
            {
                int CAV = d_vars[scan]["cav_temp"][i];

#if 1 // Experimental, take fist valid value if no value is provided
                if (CAV == 0)
                {
                    for (int scan2 = 0; scan2 < d_products->images[7].image.height() / 10; scan2++)
                    {
                        CAV = d_vars[scan2]["cav_temp"][i];
                        if (CAV != 0)
                            break;
                    }
                }
#endif

                if (CAV > 0 && CAV < 511)
                {
                    float *C_CAV;
                    if (true) // CP_index[S] == ELECTRONICS_PRIMARY)
                        C_CAV = Eng_Coeffs.C_CAV_A;
                    else
                        C_CAV = Eng_Coeffs.C_CAV_B;

                    double cavtemp = 0;
                    CONVERT_TEMP_POLY(cavtemp, CAV, C_CAV, 511, 273.15);

                    // logger->critical("CAV %d %d %f", i, CAV, cavtemp - 273.15);

                    temp += cavtemp;
                    ntemp++;
                }
            }

            if (ntemp == 0)
                return -1;

            return temp / ntemp;
        }

        double EosMODISCalibrator::get_ins_temperature(int scan)
        {
            Engineering_Coefficients_t Eng_Coeffs = Engineering_Coefficients_FM1;

            double temp = 0, ntemp = 0;
            for (int i = 0; i < 2; i++)
            {
                int INS = d_vars[scan]["inst_temp"][i];

#if 1 // Experimental, take most common value if delta is too high
                std::vector<int> v_INS;
                for (int scan2 = 0; scan2 < d_products->images[7].image.height() / 10; scan2++)
                    v_INS.push_back(d_vars[scan2]["inst_temp"][i]);

                int mINS = most_common(v_INS.begin(), v_INS.end());
                if (abs(INS - mINS) > 200)
                    INS = mINS;

                logger->trace("INS %d %d", i, INS);
#endif

                if (INS > 0 && INS < 4095)
                {
                    float *C_INS;
                    if (true) // CP_index[S] == ELECTRONICS_PRIMARY)
                        C_INS = Eng_Coeffs.C_INS_A;
                    else
                        C_INS = Eng_Coeffs.C_INS_B;

                    double instemp = 0;
                    CONVERT_TEMP_POLY(instemp, INS, C_INS, 4095, 273.15);

                    // logger->critical("INS %d %d %f", i, INS, instemp - 273.15);

                    temp += instemp;
                    ntemp++;
                }
            }

            if (ntemp == 0)
                return -1;

            return temp / ntemp;
        }

        void EosMODISCalibrator::get_fp_temperature(double *fp_temps, int scan)
        {
            Engineering_Coefficients_t Eng_Coeffs = Engineering_Coefficients_FM1;

            int FP1 = d_vars[scan]["fp_temp"][0];
            int FP2 = d_vars[scan]["fp_temp"][1];
            int FP3 = d_vars[scan]["fp_temp"][2];
            int FP4 = d_vars[scan]["fp_temp"][3];

            if (FP1 == 0 || FP1 == 4095)
                return;
            if (FP2 == 0 || FP2 == 4095)
                return;
            if (FP3 == 0 || FP3 == 4095)
                return;
            if (FP4 == 0 || FP4 == 4095)
                return;

#if 1 // Experimental. We assume it does not change during the pass!
            std::vector<bool> v_FP_T1SET;
            std::vector<bool> v_FP_T3SET;
            std::vector<bool> v_LWHTR_ON;
            std::vector<bool> v_SMHTR_ON;
            for (int scan = 0; scan < d_products->images[7].image.height() / 10; scan++)
            {
                v_FP_T1SET.push_back(d_vars[scan]["fp_temp_info"][0]);
                v_FP_T3SET.push_back(d_vars[scan]["fp_temp_info"][1]);
                v_LWHTR_ON.push_back(d_vars[scan]["fp_temp_info"][2]);
                v_SMHTR_ON.push_back(d_vars[scan]["fp_temp_info"][3]);
            }
            bool FP_T1SET = most_common(v_FP_T1SET.begin(), v_FP_T1SET.end());
            bool FP_T3SET = most_common(v_FP_T3SET.begin(), v_FP_T3SET.end());
            bool LWHTR_ON = most_common(v_LWHTR_ON.begin(), v_LWHTR_ON.end());
            bool SMHTR_ON = most_common(v_SMHTR_ON.begin(), v_SMHTR_ON.end());
#else
            bool FP_T1SET = d_vars[scan]["fp_temp_info"][0];
            bool FP_T3SET = d_vars[scan]["fp_temp_info"][1];
            bool LWHTR_ON = d_vars[scan]["fp_temp_info"][2];
            bool SMHTR_ON = d_vars[scan]["fp_temp_info"][3];
#endif

            // Compute focal plane assembly 1 and 2 temperatures.
            // The polynomial result is in degrees Celsius, so we
            // use the CTOK macro.

            CONVERT_TEMP_POLY(fp_temps[0], FP1, Eng_Coeffs.C_FP1, 4095, 273.15);
            CONVERT_TEMP_POLY(fp_temps[1], FP2, Eng_Coeffs.C_FP2, 4095, 273.15);

            // Determine FP_Set_Point_State, needed for FP_3 & _4

            int FP_SPS = 0;
            if (LWHTR_ON == 0 && SMHTR_ON == 0)
                FP_SPS = 0; /*FP heaters off, running open loop*/
            else if (FP_T1SET == 1 && FP_T3SET == 0)
                FP_SPS = 0;
            else if (FP_T1SET == 0 && FP_T3SET == 0)
                FP_SPS = 1;
            else if (FP_T1SET == 0 && FP_T3SET == 1)
                FP_SPS = 2;
            else /*temp[FP_T1SET].buffer[S] == 1 && temp[FP_T3SET].buffer[S] ==1,
                   not expected!*/
                return;

            // Keep FP_SPS
            // if (LWHTR_ON == 0 && SMHTR_ON == 0)
            //     temps->fp_set_point_state[XS] = FP_SPS;
            // else
            //     temps->fp_set_point_state[XS] = FP_SPS + 1;

            // Note that for focal planes 3 and 4, the polynomial evaluates
            // the temperature in degrees Kelvin and no conversion from
            // Celsius to Kelvin is needed.

            CONVERT_TEMP_POLY(fp_temps[2], FP3, Eng_Coeffs.C_FP3[FP_SPS], 4095, 0.);
            CONVERT_TEMP_POLY(fp_temps[3], FP4, Eng_Coeffs.C_FP4[FP_SPS], 4095, 0.);

            // logger->critical("FP %f %f %f %f", fp_temps[0] - 273.15, fp_temps[1] - 273.15, fp_temps[2] - 273.15, fp_temps[3] - 273.15);
        }

        bool EosMODISCalibrator::Calculate_Planck(float *RSR,
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
#include "modis_calibrator.h"

#include "modis_engcoefs.h"
#include "modis_defs.h"
#include "logger.h"
#include "common/utils.h"

namespace eos
{
    namespace modis
    {
        namespace precompute
        {
            double get_bb_temperature(nlohmann::json &d_vars, bool is_aqua, int scan)
            {
                Engineering_Coefficients_t Eng_Coeffs = is_aqua ? Engineering_Coefficients_FM1 : Engineering_Coefficients_PFM;

                double temp = 0, ntemp = 0;
                for (int thermistor = 0; thermistor < NUM_BB_THERMISTORS; thermistor++)
                {
                    int DN = d_vars[scan]["bb_temp"][thermistor];

#if 1 // Experimental, take most common value if delta is too high
                    std::vector<int> v_DN;
                    for (size_t scan2 = 0; scan2 < d_vars.size(); scan2++)
                        if (d_vars[scan2].contains("bb_temp"))
                            v_DN.push_back(d_vars[scan2]["bb_temp"][thermistor]);

                    int mDN = most_common(v_DN.begin(), v_DN.end(), 0);
                    if (abs(DN - mDN) > 200)
                        DN = mDN;
#endif

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

            double get_mir_temperature(nlohmann::json &d_vars, bool is_aqua, int scan)
            {
                Engineering_Coefficients_t Eng_Coeffs = is_aqua ? Engineering_Coefficients_FM1 : Engineering_Coefficients_PFM;

                double temp = 0, ntemp = 0;
                for (int i = 0; i < 2; i++)
                {
                    int MIR = d_vars[scan]["mir_temp"][i];

#if 1 // Experimental, take most common value if delta is too high
                    std::vector<int> v_MIR;
                    for (size_t scan2 = 0; scan2 < d_vars.size(); scan2++)
                        if (d_vars[scan2].contains("mir_temp"))
                            v_MIR.push_back(d_vars[scan2]["mir_temp"][i]);

                    int mMIR = most_common(v_MIR.begin(), v_MIR.end(), 0);
                    if (abs(MIR - mMIR) > 200)
                        MIR = mMIR;
#endif

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

            double get_cav_temperature(nlohmann::json &d_vars, bool is_aqua, int scan)
            {
                Engineering_Coefficients_t Eng_Coeffs = is_aqua ? Engineering_Coefficients_FM1 : Engineering_Coefficients_PFM;

                double temp = 0, ntemp = 0;
                for (int i = 0; i < 2; i++)
                {
                    int CAV = d_vars[scan]["cav_temp"][i];

#if 1 // Experimental, take fist valid value if no value is provided
                    if (CAV == 0)
                    {
                        for (size_t scan2 = 0; scan2 < d_vars.size(); scan2++)
                        {
                            if (!d_vars[scan2].contains("cav_temp"))
                                continue;
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

            double get_ins_temperature(nlohmann::json &d_vars, bool is_aqua, int scan)
            {
                Engineering_Coefficients_t Eng_Coeffs = is_aqua ? Engineering_Coefficients_FM1 : Engineering_Coefficients_PFM;

                double temp = 0, ntemp = 0;
                for (int i = 0; i < 2; i++)
                {
                    int INS = d_vars[scan]["inst_temp"][i];

#if 1 // Experimental, take most common value if delta is too high
                    std::vector<int> v_INS;
                    for (size_t scan2 = 0; scan2 < d_vars.size(); scan2++)
                        if (d_vars[scan2].contains("inst_temp") && d_vars[scan2]["inst_temp"][i] != 0)
                            v_INS.push_back(d_vars[scan2]["inst_temp"][i]);

                    int mINS = v_INS.size() == 0 ? 0 : most_common(v_INS.begin(), v_INS.end(), 0);
                    if (mINS != 0 && abs(INS - mINS) > 200)
                        INS = mINS;
#endif

                    // logger->info("INS %d", INS);

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

            void get_fp_temperature(nlohmann::json &d_vars, bool is_aqua, double *fp_temps, int scan)
            {
                Engineering_Coefficients_t Eng_Coeffs = is_aqua ? Engineering_Coefficients_FM1 : Engineering_Coefficients_PFM;

                int FP1 = d_vars[scan]["fp_temp"][0];
                int FP2 = d_vars[scan]["fp_temp"][1];
                int FP3 = d_vars[scan]["fp_temp"][2];
                int FP4 = d_vars[scan]["fp_temp"][3];

#if 1 // Experimental. We assume it does not change during the pass!
                std::vector<bool> v_FP_T1SET;
                std::vector<bool> v_FP_T3SET;
                std::vector<bool> v_LWHTR_ON;
                std::vector<bool> v_SMHTR_ON;
                for (size_t scan2 = 0; scan2 < d_vars.size(); scan2++)
                {
                    if (!d_vars[scan2].contains("fp_temp_info"))
                        continue;

                    v_FP_T1SET.push_back(d_vars[scan2]["fp_temp_info"][0]);
                    v_FP_T3SET.push_back(d_vars[scan2]["fp_temp_info"][1]);
                    v_LWHTR_ON.push_back(d_vars[scan2]["fp_temp_info"][2]);
                    v_SMHTR_ON.push_back(d_vars[scan2]["fp_temp_info"][3]);
                }
                bool FP_T1SET = most_common(v_FP_T1SET.begin(), v_FP_T1SET.end(), 0);
                bool FP_T3SET = most_common(v_FP_T3SET.begin(), v_FP_T3SET.end(), 0);
                bool LWHTR_ON = most_common(v_LWHTR_ON.begin(), v_LWHTR_ON.end(), 0);
                bool SMHTR_ON = most_common(v_SMHTR_ON.begin(), v_SMHTR_ON.end(), 0);
#else
                bool FP_T1SET = d_vars[scan]["fp_temp_info"][0];
                bool FP_T3SET = d_vars[scan]["fp_temp_info"][1];
                bool LWHTR_ON = d_vars[scan]["fp_temp_info"][2];
                bool SMHTR_ON = d_vars[scan]["fp_temp_info"][3];
#endif

#if 1 // Experimental, take most common value if delta is too high
                std::vector<int> v_FP1;
                std::vector<int> v_FP2;
                std::vector<int> v_FP3;
                std::vector<int> v_FP4;
                for (size_t scan2 = 0; scan2 < d_vars.size(); scan2++)
                {
                    if (!d_vars[scan2].contains("fp_temp"))
                        continue;

                    v_FP1.push_back(d_vars[scan2]["fp_temp"][0]);
                    v_FP2.push_back(d_vars[scan2]["fp_temp"][1]);
                    v_FP3.push_back(d_vars[scan2]["fp_temp"][2]);
                    v_FP4.push_back(d_vars[scan2]["fp_temp"][3]);
                }

                int mFP1 = most_common(v_FP1.begin(), v_FP1.end(), 0);
                int mFP2 = most_common(v_FP2.begin(), v_FP2.end(), 0);
                int mFP3 = most_common(v_FP3.begin(), v_FP3.end(), 0);
                int mFP4 = most_common(v_FP4.begin(), v_FP4.end(), 0);

                if (FP1 == 0 || FP1 == 4095 || abs(FP1 - mFP1) > 200)
                    FP1 = mFP1;
                if (FP2 == 0 || FP2 == 4095 || abs(FP2 - mFP2) > 200)
                    FP2 = mFP1;
                if (FP3 == 0 || FP3 == 4095 || abs(FP3 - mFP3) > 200)
                    FP3 = mFP3;
                if (FP4 == 0 || FP4 == 4095 || abs(FP4 - mFP4) > 200)
                    FP4 = mFP4;
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
        }
    }
}
#pragma once

#include "products/image/image_calibrator.h"
#include "modis_emiss_table.h"
// #include "modis_reflec_table.h"

namespace eos
{
    namespace modis
    {
        // Values to hold per MODIS scan, for all channels
        struct ValsPerScan
        {
            // General
            bool MS = false;
            bool valid = false;

            // Emissive
            int emissive_DN_SVs[160] = {0};
            int emissive_DN_BBs[160] = {0};

            double emissive_a0[160] = {0.0};
            double emissive_a2[160] = {0.0};
            double emissive_b1[160] = {0.0};
            float emissive_Planck_mir[160] = {0.0f};

            // Temperature
            double T_bb = 0.0;
            double T_mir = 0.0;
            double T_cav = 0.0;
            double T_ins = 0.0;
            double fp_temps[4] = {0};

            NLOHMANN_DEFINE_TYPE_INTRUSIVE(ValsPerScan,
                                           MS, valid,
                                           emissive_DN_SVs, emissive_DN_BBs,
                                           emissive_a0, emissive_a2, emissive_b1, emissive_Planck_mir,
                                           T_bb, T_mir, T_cav, T_ins, fp_temps)
        };

        // All values required for final calibration
        // Stored on the heap for Windows compatibility
        struct CalibrationVars
        {
            typedef float CalibrationVars1[2];
            typedef float CalibrationVars2[1354][2];
            CalibrationVars1 *RVS_1km_Emiss_BB;   // float RVS_1km_Emiss_BB[160][2];
            CalibrationVars1 *RVS_1km_Emiss_SV;   // float RVS_1km_Emiss_SV[160][2];
            CalibrationVars2 *RVS_1km_Emiss_EV;   // float RVS_1km_Emiss_EV[160][1354][2];
            CalibrationVars2 *sigma_RVS_Emiss_EV; // float sigma_RVS_Emiss_EV[160][1354][2];
            std::vector<ValsPerScan> scan_data;

            CalibrationVars()
            {
                RVS_1km_Emiss_BB = new CalibrationVars1[160];
                RVS_1km_Emiss_SV = new CalibrationVars1[160];
                RVS_1km_Emiss_EV = new CalibrationVars2[160];
                sigma_RVS_Emiss_EV = new CalibrationVars2[160];
            }

            CalibrationVars &operator=(const nlohmann::json &j)
            {
                scan_data = j["scan_data"];
                for (int i = 0; i < 160; i++)
                {
                    for (int x = 0; x < 2; x++)
                    {
                        RVS_1km_Emiss_BB[i][x] = j["RVS_1km_Emiss_BB"];
                        RVS_1km_Emiss_SV[i][x] = j["RVS_1km_Emiss_SV"];

                        for (int z = 0; z < 1354; z++)
                        {
                            RVS_1km_Emiss_EV[i][z][x] = j["RVS_1km_Emiss_EV"];
                            sigma_RVS_Emiss_EV[i][z][x] = j["sigma_RVS_Emiss_EV"];
                        }
                    }
                }
                return *this;
            }

            ~CalibrationVars()
            {
                delete[] RVS_1km_Emiss_BB;
                delete[] RVS_1km_Emiss_SV;
                delete[] RVS_1km_Emiss_EV;
                delete[] sigma_RVS_Emiss_EV;
            }
        };

        inline void to_json(nlohmann::json &j, const CalibrationVars &v)
        {
            j["scan_data"] = v.scan_data;
            for (int i = 0; i < 160; i++)
            {
                for (int x = 0; x < 2; x++)
                {
                    j["RVS_1km_Emiss_BB"] = v.RVS_1km_Emiss_BB[i][x];
                    j["RVS_1km_Emiss_SV"] = v.RVS_1km_Emiss_SV[i][x];

                    for (int z = 0; z < 1354; z++)
                    {
                        j["RVS_1km_Emiss_EV"] = v.RVS_1km_Emiss_EV[i][z][x];
                        j["sigma_RVS_Emiss_EV"] = v.sigma_RVS_Emiss_EV[i][z][x];
                    }
                }
            }
        }

        namespace precompute
        {
            // RVS calculation
            void calculate_rvs_correction(Coefficients_Emissive &Sat_CoeffsE, CalibrationVars &cvars);

            // SV, etc AVG
            int get_emmissive_view_avg(nlohmann::json &d_vars, std::string type, int channel, int det, int scan);

            // Temperatures
            double get_bb_temperature(nlohmann::json &d_vars, bool is_aqua, int scan);
            double get_mir_temperature(nlohmann::json &d_vars, bool is_aqua, int scan);
            double get_cav_temperature(nlohmann::json &d_vars, bool is_aqua, int scan);
            double get_ins_temperature(nlohmann::json &d_vars, bool is_aqua, int scan);
            void get_fp_temperature(nlohmann::json &d_vars, bool is_aqua, double *fp_temps, int scan);

            // Planck, to compute radiance
            bool Calculate_Planck(float *RSR, float *wl, int size, float T, float *planck);

            // Calibration coeffs
            bool get_emissive_coeffs(Coefficients_Emissive &Sat_CoeffsE, bool is_aqua, CalibrationVars &cvars, double &a0, double &a2, double &b1, float &L_sm, int DN_sv, int DN_bb, ValsPerScan &scani, int D_emiss);

            // Precompute all variables
            nlohmann::json precomputeVars(satdump::products::ImageProduct *d_products, nlohmann::json d_vars, bool is_aqua);
        };

        // The actual calibrator! - Based on the MODIS ground processing SW, re-written in many places
        class EosMODISCalibrator : public satdump::products::ImageCalibrator
        {
        private:
            bool is_aqua = false;

            CalibrationVars cvars;

            double compute_emissive(int channel, int pos_x, int pos_y, uint32_t px_val);
            double compute_reflective(int channel, int pos_x, int pos_y, uint32_t px_val);

            // Coefficients_Reflective Sat_CoeffsR; // This is WIP
            Coefficients_Emissive *Sat_CoeffsE;

            // BowTie LUTs
            std::vector<std::vector<int>> bowtie_lut_1km;

        public:
            EosMODISCalibrator(satdump::products::ImageProduct *p, nlohmann::json c)
                : satdump::products::ImageCalibrator(p, c)
            {
                is_aqua = c["is_aqua"];
                Sat_CoeffsE = new Coefficients_Emissive(c["vars"]["c_emissive"]);
                cvars = c["vars"]["cvars"];

                bowtie_lut_1km = c["bowtie_lut_1km"];
            }
            ~EosMODISCalibrator()
            {
                delete Sat_CoeffsE;
            }

            double compute(int channel, int pos_x, int pos_y, uint32_t px_val);
        };
    }
}

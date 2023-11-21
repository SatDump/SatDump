#pragma once

#include "products/image_products.h"

namespace eos
{
    namespace modis
    {
        struct ValsPerScan
        {
            // General
            bool MS;
            bool valid = false;

            // Emissive
            int emissive_DN_SVs[160];
            int emissive_DN_BBs[160];

            double emissive_a0[160];
            double emissive_a2[160];
            double emissive_b1[160];
            float emissive_Planck_mir[160];

            // Temperature
            double T_bb;
            double T_mir;
            double T_cav;
            double T_ins;
            double fp_temps[4];

            NLOHMANN_DEFINE_TYPE_INTRUSIVE(ValsPerScan,
                                           MS, valid,
                                           emissive_DN_SVs, emissive_DN_BBs,
                                           emissive_a0, emissive_a2, emissive_b1, emissive_Planck_mir,
                                           T_bb, T_mir, T_cav, T_ins, fp_temps)
        };

        class EosMODISCalibrator : public satdump::ImageProducts::CalibratorBase
        {
        private:
            nlohmann::json d_vars;

            double compute_emissive(int channel, int pos_x, int pos_y, int px_val);

        private:
            float RVS_1km_Emiss_BB[160][2];
            float RVS_1km_Emiss_SV[160][2];
            float RVS_1km_Emiss_EV[160][1354][2];
            float sigma_RVS_Emiss_EV[160][1354][2];
            void calculate_rvs_correction();

            int get_emmissive_view_avg(std::string type, int channel, int det, int scan);

            double get_bb_temperature(int scan);
            double get_mir_temperature(int scan);
            double get_cav_temperature(int scan);
            double get_ins_temperature(int scan);
            void get_fp_temperature(double *fp_temps, int scan);

            bool Calculate_Planck(float *RSR, float *wl, int size, float T, float *planck);

            bool get_coeffs(double &a0, double &a2, double &b1, float &L_sm, int DN_sv, int DN_bb, ValsPerScan &scani, int D_emiss);

            std::vector<ValsPerScan> scan_data;

        public:
            EosMODISCalibrator(nlohmann::json calib, satdump::ImageProducts *products) : satdump::ImageProducts::CalibratorBase(calib, products)
            {
                d_vars = calib["vars"];
            }

            void init();
            double compute(int channel, int pos_x, int pos_y, int px_val);
        };
    }
}
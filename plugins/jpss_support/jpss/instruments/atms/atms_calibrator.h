#pragma once

#include "products/image_products.h"
#include "atms_structs.h"

namespace jpss
{
    namespace atms
    {
        // ATMS Calibration, based on the algorithm published in the ADL software
        class JpssATMSCalibrator : public satdump::ImageProducts::CalibratorBase
        {
        private:
            ATMS_SDR_CC atmsSdrCoeffsPtr;
            nlohmann::json d_vars;

            double Calculate_Sa_target(int ich, double ang_rad, double PRTtargetRadiance, double PRTradiance);
            double Calculate_Sa_scene(int ich, double ang_rad, double taRadiance, double PRTradiance);
            void calculate_prt_temps(int ich, double &atemp_prt, double *bptemp_backup, ATMSCalibPkt &convCalData, ATMSHotCalTempPkt &hotCalData, ATMSHealtStatusPkt &engData);
            double calculate_avg_warm_cnt(int pos_y, int ich);
            double calculate_avg_cold_cnt(int pos_y, int ich);

        public:
            JpssATMSCalibrator(nlohmann::json calib, satdump::ImageProducts *products) : satdump::ImageProducts::CalibratorBase(calib, products)
            {
                d_vars = calib["vars"];
                atmsSdrCoeffsPtr = calib["sdr_cc"];
            }

            void init();
            double compute(int channel, int pos_x, int pos_y, int px_val);
        };
    }
}
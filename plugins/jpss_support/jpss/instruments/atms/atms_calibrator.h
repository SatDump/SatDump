#pragma once

#include "products/image_products.h"
#include "atms_structs.h"
#include "atms_sdr_cc.h"

#define DEG_TO_RAD (M_PI / 180.0)
#define RAD_TO_DEG (180.0 / M_PI)

namespace jpss
{
    namespace atms
    {
        const int NUM_BEAM_POSITIONS = 96;
        const int NUM_COLD_SAMPLES = 4;
        const int NUM_WARM_SAMPLES = 4;

        const double BEAM_COUNTS2DEG = 360.0 / (2 << 15);

        const int NUM_SCAN_WC = 10;
        const int NUM_SCAN_CC = 10;
        const int NUM_SCAN_PRT = 9;

        class JpssATMSCalibrator : public satdump::ImageProducts::CalibratorBase
        {
        private:
            ATMS_SDR_CC atmsSdrCoeffsPtr;
            nlohmann::json d_vars;

        public:
            JpssATMSCalibrator(nlohmann::json calib, satdump::ImageProducts *products) : satdump::ImageProducts::CalibratorBase(calib, products)
            {
                d_vars = calib["vars"];
            }

            void init()
            {
            }

            double compute(int channel, int pos_x, int pos_y, int px_val)
            {
                auto &scanWeightsPrtKav = atmsSdrCoeffsPtr.scanWeightsPrtKav;
                auto &scanWeightsPrtWg = atmsSdrCoeffsPtr.scanWeightsPrtWg;

                // Extract scene counts for this scan
                uint16_t scene_scounts[NUM_CHANNELS][NUM_BEAM_POSITIONS];
                for (int i = 0; i < NUM_BEAM_POSITIONS; i++)
                    for (int y = 0; y < NUM_CHANNELS; y++)
                        scene_scounts[y][i] = d_products->images[y - 1].image[pos_y * NUM_BEAM_POSITIONS + i];

                double angle_science[NUM_BEAM_POSITIONS];
                double angle_cold[NUM_COLD_SAMPLES];
                double angle_warm[NUM_WARM_SAMPLES];

                // Unscale the scan angles of the warm load view and space view
                // Angle counts => Degree
                // For Antenna reflector emission correction
                for (int i = 0; i < NUM_BEAM_POSITIONS; i++)
                {
                    angle_science[i] = (d_vars["beam_pos_sc"][i].get<double>() - atmsSdrCoeffsPtr.resolverOffset) * BEAM_COUNTS2DEG;

                    if (angle_science[i] < -180.0 || angle_science[i] > 180.0)
                        printf("Invalid beam angle\n");
                    else
                        angle_science[i] = angle_science[i] * DEG_TO_RAD;
                }

                for (int i = 0; i < NUM_COLD_SAMPLES; i++)
                {
                    angle_cold[i] = (d_vars["beam_pos_cc"][i].get<double>() - atmsSdrCoeffsPtr.resolverOffset) * BEAM_COUNTS2DEG;

                    if (angle_cold[i] < -180.0 || angle_cold[i] > 180.0)
                        printf("Invalid beam angle\n");
                    else
                        angle_cold[i] = angle_cold[i] * DEG_TO_RAD;
                }
                double angle_cold_mean = (angle_cold[0] + angle_cold[1] + angle_cold[2] + angle_cold[3]) / double(NUM_COLD_SAMPLES);

                for (int i = 0; i < NUM_WARM_SAMPLES; i++)
                {
                    angle_warm[i] = (d_vars["beam_pos_wc"][i].get<double>() - atmsSdrCoeffsPtr.resolverOffset) * BEAM_COUNTS2DEG;

                    if (angle_warm[i] < -180.0 || angle_warm[i] > 180.0)
                        printf("Invalid beam angle\n");
                    else
                        angle_warm[i] = angle_warm[i] * DEG_TO_RAD;
                }
                double angle_warm_mean = (angle_warm[0] + angle_warm[1] + angle_warm[2] + angle_warm[3]) / double(NUM_WARM_SAMPLES);

                // Warm Counts
                double crw_[NUM_CHANNELS][NUM_WARM_SAMPLES][NUM_SCAN_WC];
                // Cold Counts
                double crc_[NUM_CHANNELS][NUM_COLD_SAMPLES][NUM_SCAN_CC];

                // Loop over appropriate scans and fill the WC arrays
                for (int c = 0; c < NUM_CHANNELS; c++)
                    for (int z = 0; z < NUM_WARM_SAMPLES; z++)
                        for (int i = 0; i < NUM_SCAN_WC; i++)
                            crw_[c][z][i] = d_vars["warm_counts"][pos_y][c][z];

                // Loop over appropriate scans and fill the CC arrays
                for (int c = 0; c < NUM_CHANNELS; c++)
                    for (int z = 0; z < NUM_COLD_SAMPLES; z++)
                        for (int i = 0; i < NUM_SCAN_CC; i++)
                            crc_[c][z][i] = d_vars["cold_counts"][pos_y][c][z];

                // Get warm load PRT counts for the current scan
                // and scans around it.
                int prtw_kav[NUM_PRT_KAV][NUM_SCAN_PRT];
                int prtw_wg[NUM_PRT_WG][NUM_SCAN_PRT];

                ATMSHotCalTempPkt hot_cal = d_vars[pos_y]["hotcal_pkt"];

                for (int c = 0; c < NUM_PRT_KAV; c++)
                    for (int i = 0; i < NUM_SCAN_PRT; i++)
                        prtw_kav[c][i] = hot_cal.kavPrt[c];
                for (int c = 0; c < NUM_PRT_WG; c++)
                    for (int i = 0; i < NUM_SCAN_PRT; i++)
                        prtw_wg[c][i] = hot_cal.wqPrt[c];

                return -1;
            }
        };
    }
}
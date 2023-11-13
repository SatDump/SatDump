#include "atms_calibrator.h"
#include "common/calibration.h"
#include "logger.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEG_TO_RAD (M_PI / 180.0)
#define RAD_TO_DEG (180.0 / M_PI)

namespace jpss
{
    namespace atms
    {
        const double BEAM_COUNTS2DEG = 360.0 / (2 << 15);

        void JpssATMSCalibrator::init()
        {
        }

        double JpssATMSCalibrator::compute(int channel, int pos_x, int pos_y, int px_val)
        {
            int scnt = px_val;                                             // Scene counts
            int ich = channel;                                             // Channel ID
            ATMSCalibPkt convCalData = d_vars[pos_y]["calibration_pkt"];   // Calibration Packet
            ATMSHotCalTempPkt hotCalData = d_vars[pos_y]["hotcal_pkt"];    // Hot Calibration Packet
            ATMSHealtStatusPkt engData = d_vars[pos_y]["engineering_pkt"]; // Engineering Packet

            // Check Data
            if (!convCalData.valid)
                return CALIBRATION_INVALID_VALUE;
            if (!hotCalData.valid)
                return CALIBRATION_INVALID_VALUE;
            if (!engData.valid)
                return CALIBRATION_INVALID_VALUE;
            if (px_val == 0)
                return CALIBRATION_INVALID_VALUE;

            // Frequency of the current channel
            double cfreq = atmsSdrCoeffsPtr.centralFrequency[ich];

            // Calculate warm & shelf PRTs
            double warm_prt = 0;
            double shelf_prts[2];

            calculate_prt_temps(ich, warm_prt, shelf_prts, convCalData, hotCalData, engData);

            if (warm_prt == 0)
                return CALIBRATION_INVALID_VALUE;

            // Always retrieve shelf PRT temperature from the backup
            double base_plate_temp = shelf_prts[1];

            // Transform vshelf prt to radiance
            double prt_vshelf_corr = atmsSdrCoeffsPtr.radianceBandCorrection[0][ich] + atmsSdrCoeffsPtr.radianceBandCorrection[1][ich] * (base_plate_temp + atmsSdrCoeffsPtr.reflectorTempOffset);
            double prt_vshelf_rad = temperature_to_radiance(prt_vshelf_corr, freq_to_wavenumber(cfreq));

            // Calculate average warm and cold counts
            double cw = calculate_avg_warm_cnt(pos_y, ich);
            double cc = calculate_avg_cold_cnt(pos_y, ich);

            // Calculate average warm/cold counts angles
            double angc_mean = 0;
            int angc_cnt = 0;
            for (int i = 0; i < NUM_COLD_SAMPLES; i++)
            {
                if (!d_vars[pos_y].contains("beam_pos_cc"))
                    continue;
                if ((int)d_vars[pos_y]["beam_pos_cc"].size() <= i)
                    continue;

                double angle_cold = (d_vars[pos_y]["beam_pos_cc"][i].get<double>() - atmsSdrCoeffsPtr.resolverOffset) * BEAM_COUNTS2DEG;

                if (angle_cold < -180.0 || angle_cold > 180.0)
                    logger->trace("ATMS Calibratior : Invalid cold beam angle %f", angle_cold);
                else
                {
                    angc_mean += angle_cold * DEG_TO_RAD;
                    angc_cnt++;
                }
            }
            if (angc_cnt == 0)
                return CALIBRATION_INVALID_VALUE;
            angc_mean /= double(angc_cnt);

            double angw_mean = 0;
            int angw_cnt = 0;
            for (int i = 0; i < NUM_WARM_SAMPLES; i++)
            {
                if (!d_vars[pos_y].contains("beam_pos_wc"))
                    continue;
                if ((int)d_vars[pos_y]["beam_pos_wc"].size() <= i)
                    continue;

                double angle_warm = (d_vars[pos_y]["beam_pos_wc"][i].get<double>() - atmsSdrCoeffsPtr.resolverOffset) * BEAM_COUNTS2DEG;

                if (angle_warm < -180.0 || angle_warm > 180.0)
                    logger->trace("ATMS Calibratior : Invalid warm beam angle %f", angle_warm);
                else
                {
                    angw_mean += angle_warm * DEG_TO_RAD;
                    angw_cnt++;
                }
            }
            if (angw_cnt == 0)
                return CALIBRATION_INVALID_VALUE;
            angw_mean /= double(angw_cnt);

            // Include biases to compute warm load, cold space temperatures
            // The warm bias can be from telemetry or from parameter input
            double warm_bias = convCalData.warmBias[ich];
            double tw = warm_prt + warm_bias;

            // In radiance processing, cold space temperature is set to 2.73K
            double cold_bias = convCalData.coldBias[ich];
            double tc = 2.73 + cold_bias;

            // get warmload radiance
            double twcorr = atmsSdrCoeffsPtr.radianceBandCorrection[0][ich] + atmsSdrCoeffsPtr.radianceBandCorrection[1][ich] * tw;

            double tw_rad = temperature_to_radiance(twcorr, freq_to_wavenumber(cfreq));

            // Antenna reflector emission correction in radiance
            tw_rad = tw_rad + Calculate_Sa_target(ich, angw_mean, tw_rad, prt_vshelf_rad);

            // get cold space radiance
            double tccorr = atmsSdrCoeffsPtr.radianceBandCorrection[0][ich] + atmsSdrCoeffsPtr.radianceBandCorrection[1][ich] * tc;

            double tc_rad = temperature_to_radiance(tccorr, freq_to_wavenumber(cfreq));

            // Antenna reflector emission correction in radiance
            tc_rad = tc_rad + Calculate_Sa_target(ich, angc_mean, tc_rad, prt_vshelf_rad);

            // calculate gain in counts/radiance
            double gain_rad = (cw - cc) / (tw_rad - tc_rad);

            // calculate linear radiance
            double tk_rad = tw_rad + ((scnt - cw) / gain_rad);

            // Get Earth beam angle
            if (!d_vars[pos_y].contains("beam_pos_sc"))
                return CALIBRATION_INVALID_VALUE;
            if ((int)d_vars[pos_y]["beam_pos_sc"].size() <= pos_x)
                return CALIBRATION_INVALID_VALUE;

            double angle_sc = (d_vars[pos_y]["beam_pos_sc"][pos_x].get<double>() - atmsSdrCoeffsPtr.resolverOffset) * BEAM_COUNTS2DEG;

            if (angle_sc < -180.0 || angle_sc > 180.0)
                logger->trace("ATMS Calibratior : Invalid Earth beam angle %f", angle_sc);
            else
                angle_sc += angle_sc * DEG_TO_RAD;

            double degc = base_plate_temp;

            // Use of the quadratic term can be enabled or disabled in the
            // ancillary data file.
            // nonlinearity correction using
            if (atmsSdrCoeffsPtr.useQuadraticTerm != 0)
            {
                int rc_num = engData.data[72] & 0b111;
                int mapped_rc = atmsSdrCoeffsPtr.mapRc[rc_num];

                double quadratic_factor = 0;

                // normalize scene counts to 0~1
                double cnorm = (scnt - cc) / (cw - cc);
                if (degc <= -999.9 + 0.001)
                {
                    quadratic_factor = 0;
                }
                else
                {
                    // calculate mu parameters from receiver temperature
                    double mupar = atmsSdrCoeffsPtr.quadraticRc[0][mapped_rc][ich] * pow(degc, 2) +
                                   atmsSdrCoeffsPtr.quadraticRc[1][mapped_rc][ich] * degc +
                                   atmsSdrCoeffsPtr.quadraticRc[2][mapped_rc][ich];
                    // calculate Tnlmax from mu
                    quadratic_factor = 0.25 * mupar * pow((tw_rad - tc_rad), 2);
                }

                // get nonlinearity in Tb
                double qterm = quadratic_factor * (4 * pow((cnorm - 0.5), 2) - 1);
                double tk_rad = tk_rad + qterm;

                // Antenna reflector emission correction for scene
                tk_rad = Calculate_Sa_scene(ich, angle_sc, tk_rad, prt_vshelf_rad);
            }

            // Earth scene antenna pattern correction in radiance
            double tkb_rad = atmsSdrCoeffsPtr.beamEfficiencyCorrection[ich][95 - pos_x] * tk_rad + atmsSdrCoeffsPtr.scanBias[ich][95 - pos_x] * tc_rad;

            return tkb_rad;
        }

        double JpssATMSCalibrator::Calculate_Sa_target(int ich, double ang_rad, double PRTtargetRadiance, double PRTradiance)
        {
            double reflectorRadiance = PRTradiance;

            double eps = atmsSdrCoeffsPtr.reflectorEmissivity[ich]; // emissivity
            double TwistAngle = 0.0;                                // twistangle

            if (ich == 1 || ich == 2 || ich == 16)
            {
                double sin2theta = pow((sin(ang_rad + TwistAngle)), 2);
                return eps * (reflectorRadiance - PRTtargetRadiance) + (1 - eps) * eps * (reflectorRadiance - PRTtargetRadiance) * sin2theta;
            }
            else
            {
                double cos2theta = pow((cos(ang_rad + TwistAngle)), 2);
                return eps * (reflectorRadiance - PRTtargetRadiance) + (1 - eps) * eps * (reflectorRadiance - PRTtargetRadiance) * cos2theta;
            }
        }

        double JpssATMSCalibrator::Calculate_Sa_scene(int ich, double ang_rad, double taRadiance, double PRTradiance)
        {
            // 2W PRT V-Shelf radiance used as reflector radiance
            double reflectorRadiance = PRTradiance;
            double var1, var2;

            double eps = atmsSdrCoeffsPtr.reflectorEmissivity[ich]; // emissivity
            double TwistAngle = 0.0;                                // ScanBiasCoeff%beta1(ich)*deg2rad !twistangle
            double eh = eps;
            double sin2theta = pow((sin(ang_rad + TwistAngle)), 2);
            double cos2theta = pow((cos(ang_rad + TwistAngle)), 2);
            if (ich == 1 || ich == 2 || ich == 16)
            {
                var1 = (1 - eh) * (1 - eh * sin2theta);
                var2 = eh * (1 + (1 - eh) * sin2theta);
            }
            else
            {
                var1 = (1 - eh) * (1 - eh * cos2theta);
                var2 = eh * (1 + (1 - eh) * cos2theta);
            }
            double A = 1 / var1;
            double B = -var2 / var1;
            return A * taRadiance + reflectorRadiance * B;
        }

        void JpssATMSCalibrator::calculate_prt_temps(int ich, double &atemp_prt, double *bptemp_backup, ATMSCalibPkt &convCalData, ATMSHotCalTempPkt &hotCalData, ATMSHealtStatusPkt &engData)
        {
            ///////////////////////////////////////// Calculate warm PRTs
            int num_prt = ich < 15 ? NUM_PRT_KAV : NUM_PRT_WG;
            double rref = ich < 15 ? convCalData.pamKav : convCalData.pamWg;
            uint16_t *prtw = ich < 15 ? (uint16_t *)hotCalData.kavPrt : (uint16_t *)hotCalData.wqPrt;
            double coff = engData.data[45]; // INDEX_46;
            double cref = ich < 15 ? hotCalData.kavPamCounts : hotCalData.wgPamCounts;

            double num_valid_prts = 0;
            for (int i = 0; i < num_prt; i++)
            {
                double temp_prt = -1;

                double r0 = ich < 15 ? convCalData.prtCoeffKav[i][0] : convCalData.prtCoeffWg[i][0];
                double alpha = ich < 15 ? convCalData.prtCoeffKav[i][1] : convCalData.prtCoeffWg[i][1];
                double delta = ich < 15 ? convCalData.prtCoeffKav[i][2] : convCalData.prtCoeffWg[i][2];
                double beta = ich < 15 ? convCalData.prtCoeffKav[i][3] : convCalData.prtCoeffWg[i][3];
                double r0a = r0 * alpha;

                if (r0 == 0)
                {
                    logger->trace("ATMS Calibratior : Bad PRT!");
                    continue; // Bad PRT
                }

                // Compute PRT resistance
                double ri = (rref * (double(prtw[i]) - coff)) / (cref - coff);

                //  Setup initial guess for temperature (degress C)
                double tguess = ((ri - r0) / r0a) - 1;
                double degc = tguess + atmsSdrCoeffsPtr.prtConvergence + 1;

                // Loop using Newton-Raphson method until temperature converges -
                // this should happen very quickly.
                int iloop = 0;
                bool converged = true;
                while (abs(degc - tguess) > atmsSdrCoeffsPtr.prtConvergence && converged)
                {
                    tguess = degc;
                    double tg100 = tguess / 100.0;
                    double ft = r0 - ri + (r0a * (tguess - (delta * (tg100 - 1.0) * tg100) - (beta * (tg100 - 1.0) * (pow(tg100, 3)))));
                    double ftprime = r0a * (1.0 - (delta * ((tguess / 5000.0) - 0.01)) - (beta * (((pow(tguess, 3)) / 2.5e7) - (3.0 * (pow(tguess, 2)) / 1.0e6))));
                    degc = tguess - (ft / ftprime);
                    // Data Integrity: check for infinite loop
                    iloop++;
                    if (iloop > atmsSdrCoeffsPtr.prtLoops)
                    {
                        logger->trace("ATMS Calibratior : PRT Temp not converging!");
                        converged = false;
                    }
                }

                if (converged)
                    temp_prt = degc + 273.15;

                if (temp_prt != -1)
                {
                    num_valid_prts++;
                    atemp_prt += temp_prt;
                }
            }

            // Compute average
            atemp_prt /= double(num_valid_prts);

            ///////////////////////////////////////// Calculate shelf PRTs
            for (int i = 0; i < 2; i++)
            {
                bptemp_backup[i] = -1;

                int ioff = ich < 15 ? 0 : 2;

                double prt_bp[2] = {ich < 15 ? (double)engData.data[26] : (double)engData.data[25],
                                    ich < 15 ? (double)engData.data[28] : (double)engData.data[27]};

                double r0 = convCalData.prtCoeffShelf[i + ioff][0];
                double alpha = convCalData.prtCoeffShelf[i + ioff][1];
                double delta = convCalData.prtCoeffShelf[i + ioff][2];
                double beta = convCalData.prtCoeffShelf[i + ioff][3];
                double r0a = r0 * alpha;

                if (r0 == 0)
                {
                    logger->trace("ATMS Calibratior : Bad PRT!");
                    continue; // Bad PRT
                }

                // Compute PRT resistance
                double ri = (rref * (double(prt_bp[i]) - coff)) / (cref - coff);

                //  Setup initial guess for temperature (degress C)
                double tguess = ((ri - r0) / r0a) - 1;
                double degc = tguess + atmsSdrCoeffsPtr.prtConvergence + 1;

                // Loop using Newton-Raphson method until temperature converges -
                // this should happen very quickly.
                int iloop = 0;
                bool converged = true;
                while (abs(degc - tguess) > atmsSdrCoeffsPtr.prtConvergence && converged)
                {
                    tguess = degc;
                    double tg100 = tguess / 100.0;
                    double ft = r0 - ri + (r0a * (tguess - (delta * (tg100 - 1.0) * tg100) - (beta * (tg100 - 1.0) * (pow(tg100, 3)))));
                    double ftprime = r0a * (1.0 - (delta * ((tguess / 5000.0) - 0.01)) - (beta * (((pow(tguess, 3)) / 2.5e7) - (3.0 * (pow(tguess, 2)) / 1.0e6))));
                    degc = tguess - (ft / ftprime);
                    // Data Integrity: check for infinite loop
                    iloop++;
                    if (iloop > atmsSdrCoeffsPtr.prtLoops)
                    {
                        logger->trace("ATMS Calibratior : Shelf PRT Temp not converging!");
                        converged = false;
                    }
                }

                if (converged)
                {
                    if (degc < atmsSdrCoeffsPtr.shelfTemp[0][i + ioff])
                        bptemp_backup[i] = atmsSdrCoeffsPtr.shelfTemp[0][i + ioff] + 273.15;
                    else if (degc > atmsSdrCoeffsPtr.shelfTemp[2][i + ioff])
                        bptemp_backup[i] = atmsSdrCoeffsPtr.shelfTemp[2][i + ioff] + 273.15;
                    else
                        bptemp_backup[i] = degc + 273.15;
                }
            }
        }

        double JpssATMSCalibrator::calculate_avg_warm_cnt(int pos_y, int ich)
        {
            int num_valid = 0;
            double favg = -1;
            for (int i = 0; i < 4; i++)
            {
                double v = d_vars[pos_y]["warm_counts"][ich][i];
                if (v != 0)
                {
                    num_valid++;
                    favg += v;
                }
            }
            if (num_valid > 0)
                return favg / double(num_valid);
            else
                return -1;
        }

        double JpssATMSCalibrator::calculate_avg_cold_cnt(int pos_y, int ich)
        {
            int num_valid = 0;
            double favg = -1;
            for (int i = 0; i < 4; i++)
            {
                double v = d_vars[pos_y]["cold_counts"][ich][i];
                if (v != 0)
                {
                    num_valid++;
                    favg += v;
                }
            }
            if (num_valid > 0)
                return favg / double(num_valid);
            else
                return -1;
        }
    }
}
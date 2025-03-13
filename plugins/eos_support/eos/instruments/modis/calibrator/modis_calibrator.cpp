#include "modis_calibrator.h"
#include "common/calibration.h"
#include "logger.h"
#include "modis_defs.h"
#include "common/utils.h"

namespace eos
{
    namespace modis
    {
        double EosMODISCalibrator::compute(int channel, int pos_x, int pos_y, uint32_t px_val)
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

        double EosMODISCalibrator::compute_emissive(int channel, int pos_x, int pos_y, uint32_t px_val)
        {
            int index_channel = channel;
            channel -= NUM_250M_BANDS + NUM_500M_BANDS + NUM_1000M_REFL_BANDS - 1; // Scale channel to "index in category"
            // int index_channel_em = channel;

            // Skip Band 26
            if (channel == MODIS_BAND26_INDEX_AT_RES)
                return CALIBRATION_INVALID_VALUE;
            if (channel >= MODIS_BAND26_INDEX_AT_RES)
                channel--;

            auto &scaninfo = cvars.scan_data[pos_y / 10];

            // Calculate & Check variables we need
            if (!scaninfo.valid)
            {
                logger->error("Scan Invalid!\n");
                return CALIBRATION_INVALID_VALUE;
            }

            bool MS = scaninfo.MS; // Mirror side

            // We need the *actual* detector, before bowtie correction!
            int spos_y = pos_y % 10;
            if (bowtie_lut_1km.size() > 0)
            {
                if ((size_t)pos_x >= bowtie_lut_1km.size())
                    return CALIBRATION_INVALID_VALUE;
                if ((size_t)spos_y >= bowtie_lut_1km[pos_x].size())
                    return CALIBRATION_INVALID_VALUE;
                spos_y = bowtie_lut_1km[pos_x][spos_y];
            }

            int D_emiss = channel * 10 + (9 - spos_y);     // Index of emissive detector
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
                logger->error("Coef Invalid!\n");
                return CALIBRATION_INVALID_VALUE;
            }

            // logger->warn("HEY! %d --- SV %d BB %d EV %d --- BBT %f CAV %f MIR %f INS %f --- dn_bb %d - dn_ev %d - deltaT_bb %f --- a0 %f a2 %f b1 %f -- BBCorr %f SVCorr %f",
            //              D_emiss, DN_sv, DN_bb, DN_ev,
            //              T_bb - 273.15, T_cav - 273.15, T_mir - 273.15, T_ins - 273.15,
            //              dn_bb, dn_ev, T_bb + delta_T_bb - 273.15,
            //              a0, a2, b1,
            //              bb_corr, sv_corr);

#if 0
            // Apply PCX if needed
            if (Sat_CoeffsE.PCX_correction_switch[0] == 1)
            {
                int D = D_emiss % 10;
                int &B = index_channel_em;

                if (B >= 12 && B <= 16)
                { // pc xtalk correction
                    int B_xt = B - 12;
                    int F_shift = F + Sat_CoeffsE.PC_XT[B_xt][D][1];
                    if (F_shift >= 0 && F_shift < EV_1km_FRAMES)
                    {
                        int dn_ev_31 = d_products->images[32].image[pos_y * EV_1km_FRAMES + pos_x] - scaninfo.emissive_DN_SVs[100 + D];
                        if (dn_ev_31 > 0)
                        {
                            double dn_XT_corr = -Sat_CoeffsE.PC_XT[B_xt][D][0] * 0.01 * dn_ev_31 * Sat_CoeffsE.PC_XT[B_xt][D][2] + Sat_CoeffsE.PC_XT[B_xt][D][3];
                            dn_ev += dn_XT_corr;
                        }
                    }
                }
            }
#endif

            // Compute L_ev
            double Fn = a0 +
                        b1 * dn_ev +
                        a2 * dn_ev * dn_ev;

            // Compute radiance
            double L_ev = (Fn -
                           (cvars.RVS_1km_Emiss_SV[D_emiss][MS] - cvars.RVS_1km_Emiss_EV[D_emiss][F][MS]) *
                               /*PP_emiss->Planck_mir[D_emiss][S]*/ L_sm) /
                          cvars.RVS_1km_Emiss_EV[D_emiss][F][MS];

            double radiance = spectral_radiance_to_radiance(L_ev, d_pro->get_channel_wavenumber(index_channel));
            if (std::isnan(radiance))
                return CALIBRATION_INVALID_VALUE;
            else
                return radiance;
        }

        double EosMODISCalibrator::compute_reflective(int /* channel */, int /* pos_x */, int /* pos_y */, uint32_t /* px_val */)
        {
            return CALIBRATION_INVALID_VALUE;
        }
    }
}
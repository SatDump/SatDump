#include "atms_reader.h"
#include "common/ccsds/ccsds_time.h"

namespace jpss
{
    namespace atms
    {
        ATMSReader::ATMSReader()
        {
            for (int i = 0; i < 22; i++)
            {
                channels[i].resize(96);
                channels_cc[i].resize(4);
                channels_wc[i].resize(4);
            }
            lines = 0;
            scan_pos = -1;
        }

        ATMSReader::~ATMSReader()
        {
            for (int i = 0; i < 22; i++)
                channels[i].clear();
        }

        void ATMSReader::work(ccsds::CCSDSPacket &packet)
        {
            // Filter out bad packets
            if (packet.payload.size() < 56)
                return;

            // Is there a sync signal?
            int scan_synch = packet.payload[10] >> 7;

            // If there is, trigger a new scanline
            if (scan_synch == 1)
            {
                lines++;
                timestamps.push_back(ccsds::parseCCSDSTimeFull(packet, -4383));
                scan_pos = 0;

                for (int i = 0; i < 22; i++)
                {
                    channels[i].resize((lines + 1) * 96);
                    channels_cc[i].resize((lines + 1) * 4);
                    channels_wc[i].resize((lines + 1) * 4);
                }
            }

            // Safeguard
            if (scan_pos < 96 && scan_pos >= 0)
                for (int i = 0; i < 22; i++) // Decode all channels
                    channels[i][(lines * 96) + 95 - scan_pos] = (packet.payload[12 + i * 2] << 8 | packet.payload[13 + i * 2]);
            else if (scan_pos - 96 < 4 && scan_pos - 96 >= 0)
                for (int i = 0; i < 22; i++) // Decode all channels
                    channels_cc[i][(lines * 4) + (scan_pos - 96)] = (packet.payload[12 + i * 2] << 8 | packet.payload[13 + i * 2]);
            else if (scan_pos - 96 - 4 < 4 && scan_pos - 96 - 4 >= 0)
                for (int i = 0; i < 22; i++) // Decode all channels
                    channels_wc[i][(lines * 4) + (scan_pos - 96 - 4)] = (packet.payload[12 + i * 2] << 8 | packet.payload[13 + i * 2]);

            scan_pos++;
        }

        inline double parse_pam_gamma0(uint16_t word) { return 2300.0 + 0.006 * word; }
        inline double parse_4w_2w_r0(uint16_t word) { return 1900.0 + 0.003 * word; }
        inline double parse_4w_2w_alpha(uint16_t word) { return 0.002 + 5e-8 * word; }
        inline double parse_4w_2w_delta(uint16_t word) { return 5e-5 * word; }
        inline double parse_4w_prt_beta(uint16_t word) { return 6e-5 * word - 2.0; }
        inline double parse_calibration_target_offset(uint16_t word) { return -7.5e-6 * word; }
        inline double parse_cold_calibration_offset(uint16_t word) { return 1.5e-5 * word; }
        inline double parse_quadratic_coefficient(uint16_t word) { return 2.6e-5 * word - 0.850; }
        inline double parse_alignement(uint16_t word) { return 2.0e-5 * word - 0.655; }
        inline double parse_2w_prt_a(uint16_t word) { return 3.0e-6 * word; }
        inline double parse_rc(uint16_t word) { return 0.0003 * word; }
        inline double parse_muxresti(uint16_t word) { return 1900.0 + 0.003 * word; }

        struct CalTargetTemp
        {
            double r0;
            double alpha;
            double delta;
            double beta;
        };

        struct ShelfTemp
        {
            double r0;
            double alpha;
            double delta;
            double rc;
        };

        inline CalTargetTemp parse_cal_target_temp(uint16_t *dat)
        {
            return {parse_4w_2w_r0(dat[0]), parse_4w_2w_alpha(dat[1]), parse_4w_2w_delta(dat[2]), parse_4w_prt_beta(dat[3])};
        }

        inline ShelfTemp parse_shelf_temp(uint16_t *dat)
        {
            return {parse_4w_2w_r0(dat[0]), parse_4w_2w_alpha(dat[1]), parse_4w_2w_delta(dat[2]), parse_rc(dat[3])};
        }

        void ATMSReader::work_calib(ccsds::CCSDSPacket &packet)
        {
            // Filter out bad packets
            if (packet.payload.size() < 438)
                return;

            uint16_t *words = (uint16_t *)&packet.payload[8];

            double k_ka_v_pam_resistance = parse_pam_gamma0(words[0]);
            double w_g_pam_resistance = parse_pam_gamma0(words[1]);

            CalTargetTemp prt_kkav[8];
            for (int i = 0; i < 8; i++)
                prt_kkav[i] = parse_cal_target_temp(&words[2 + 4 * i]);

            CalTargetTemp prt_wg[7];
            for (int i = 0; i < 7; i++)
                prt_wg[i] = parse_cal_target_temp(&words[2 + 4 * 8 + 4 * i]);

            double k_cal_target_offset = parse_calibration_target_offset(words[62 + 0]);
            double ka_cal_target_offset = parse_calibration_target_offset(words[62 + 1]);
            double v_cal_target_offset = parse_calibration_target_offset(words[62 + 2]);
            double w_cal_target_offset = parse_calibration_target_offset(words[62 + 3]);
            double g_cal_target_offset = parse_calibration_target_offset(words[62 + 4]);

            double k_cal_cold_offset = parse_cold_calibration_offset(words[67 + 0]);
            double ka_cal_cold_offset = parse_cold_calibration_offset(words[67 + 1]);
            double v_cal_cold_offset = parse_cold_calibration_offset(words[67 + 2]);
            double w_cal_cold_offset = parse_cold_calibration_offset(words[67 + 3]);
            double g_cal_cold_offset = parse_cold_calibration_offset(words[67 + 4]);

            double quadratic_coefficients[22];
            for (int i = 0; i < 22; i++)
                quadratic_coefficients[i] = parse_quadratic_coefficient(words[72 + i]);

            ShelfTemp k_band_shelf_temp = parse_shelf_temp(&words[139 + 4 * 0]);
            ShelfTemp v_band_shelf_temp = parse_shelf_temp(&words[139 + 4 * 1]);
            ShelfTemp w_band_shelf_temp = parse_shelf_temp(&words[139 + 4 * 2]);
            ShelfTemp g_band_shelf_temp = parse_shelf_temp(&words[139 + 4 * 3]);
        }

        image::Image<uint16_t> ATMSReader::getChannel(int channel)
        {
            return image::Image<uint16_t>(channels[channel].data(), 96, lines, 1);
        }
    } // namespace atms
} // namespace jpss
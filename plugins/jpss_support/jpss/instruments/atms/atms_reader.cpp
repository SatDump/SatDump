#include "atms_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"

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
                calib_data[lines]["calibration_pkt"] = last_calib_pkt;
                calib_data[lines]["engineering_pkt"] = last_eng_pkt;
                calib_data[lines]["hotcal_pkt"] = last_hot_pkt;

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

            int16_t beam_angle;
            ((uint8_t *)&beam_angle)[1] = packet.payload[8 + 0];
            ((uint8_t *)&beam_angle)[0] = packet.payload[8 + 1];

            // Safeguard
            if (scan_pos < 96 && scan_pos >= 0)
            {
                for (int i = 0; i < 22; i++) // Decode all channels
                    channels[i][(lines * 96) + 95 - scan_pos] = (packet.payload[12 + i * 2] << 8 | packet.payload[13 + i * 2]);
                calib_data[lines]["beam_pos_sc"][95 - scan_pos] = beam_angle;
            }
            else if (scan_pos - 96 < 4 && scan_pos - 96 >= 0)
            {
                for (int i = 0; i < 22; i++) // Decode all channels
                    channels_cc[i][(lines * 4) + (scan_pos - 96)] = (packet.payload[12 + i * 2] << 8 | packet.payload[13 + i * 2]);
                calib_data[lines]["beam_pos_cc"][scan_pos - 96] = beam_angle;
            }
            else if (scan_pos - 96 - 4 < 4 && scan_pos - 96 - 4 >= 0)
            {
                for (int i = 0; i < 22; i++) // Decode all channels
                    channels_wc[i][(lines * 4) + (scan_pos - 96 - 4)] = (packet.payload[12 + i * 2] << 8 | packet.payload[13 + i * 2]);
                calib_data[lines]["beam_pos_wc"][scan_pos - 96 - 4] = beam_angle;
            }

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

        void ATMSReader::work_calib(ccsds::CCSDSPacket &packet)
        {
            // Filter out bad packets
            if (packet.payload.size() < 438)
                return;

            // Parse calibration packet
            ATMSCalibPkt calib_pkt;
            uint16_t words[215];
            repackBytesTo16bits(&packet.payload[8], 430, words);

            calib_pkt.pamKav = parse_pam_gamma0(words[0]);
            calib_pkt.pamWg = parse_pam_gamma0(words[1]);

            for (int i = 0; i < NUM_PRT_KAV; i++)
            {
                calib_pkt.prtCoeffKav[i][0] = parse_4w_2w_r0(words[2 + 4 * i + 0]);
                calib_pkt.prtCoeffKav[i][1] = parse_4w_2w_alpha(words[2 + 4 * i + 1]);
                calib_pkt.prtCoeffKav[i][2] = parse_4w_2w_delta(words[2 + 4 * i + 2]);
                calib_pkt.prtCoeffKav[i][3] = parse_4w_prt_beta(words[2 + 4 * i + 3]);
            }

            for (int i = 0; i < NUM_PRT_WG; i++)
            {
                calib_pkt.prtCoeffWg[i][0] = parse_4w_2w_r0(words[34 + 4 * i + 0]);
                calib_pkt.prtCoeffWg[i][1] = parse_4w_2w_alpha(words[34 + 4 * i + 1]);
                calib_pkt.prtCoeffWg[i][2] = parse_4w_2w_delta(words[34 + 4 * i + 2]);
                calib_pkt.prtCoeffWg[i][3] = parse_4w_prt_beta(words[34 + 4 * i + 3]);
            }

            for (int i = 0; i < NUM_ATMS_BANDS; i++)
                calib_pkt.warmBias[i] = parse_calibration_target_offset(words[62 + i]);

            for (int i = 0; i < NUM_ATMS_BANDS; i++)
                calib_pkt.coldBias[i] = parse_cold_calibration_offset(words[67 + i]);

            for (int i = 0; i < 22; i++)
                calib_pkt.quadraticCoeffs[i] = parse_quadratic_coefficient(words[72 + i]);

            // Beam alignement - Skipped

            for (int i = 0; i < NUM_PRT_COEFFS; i++)
            {
                calib_pkt.prtCoeffShelf[i][0] = parse_4w_2w_r0(words[139 + 4 * i + 0]);
                calib_pkt.prtCoeffShelf[i][1] = parse_4w_2w_alpha(words[139 + 4 * i + 1]);
                calib_pkt.prtCoeffShelf[i][2] = parse_4w_2w_delta(words[139 + 4 * i + 2]);
                calib_pkt.prtCoeffShelf[i][3] = parse_rc(words[139 + 4 * i + 3]);
            }

            for (int i = 0; i < NUM_PRT_COEFF_2WIRE / 2; i++)
            {
                calib_pkt.prtCoeff2Wire[i * 2 + 0] = parse_4w_2w_r0(words[155 + 2 * i + 0]);
                calib_pkt.prtCoeff2Wire[i * 2 + 1] = parse_2w_prt_a(words[155 + 2 * i + 1]);
            }

            for (int i = 0; i < NUM_HOUSE_KEEPING; i++)
            {
                calib_pkt.houseKeeping[i] = parse_muxresti(words[211 + i]);
            }

            calib_pkt.valid = true;
            last_calib_pkt = calib_pkt;
        }

        void ATMSReader::work_eng(ccsds::CCSDSPacket &packet)
        {
            // Filter out bad packets
            if (packet.payload.size() < 156)
                return;

            ATMSHealtStatusPkt eng_pkt;
            uint16_t words[74];
            repackBytesTo16bits(&packet.payload[8], 148, words);

            for (int i = 0; i < NUM_HS_VARS; i++)
                eng_pkt.data[i] = words[i];

            eng_pkt.valid = true;
            last_eng_pkt = eng_pkt;
        }

        void ATMSReader::work_hotcal(ccsds::CCSDSPacket &packet)
        {
            // Filter out bad packets
            if (packet.payload.size() < 42)
                return;

            ATMSHotCalTempPkt hot_pkt;
            uint16_t words[17];
            repackBytesTo16bits(&packet.payload[8], 34, words);

            for (int i = 0; i < NUM_PRT_KAV; i++)
                hot_pkt.kavPrt[i] = words[i];

            hot_pkt.kavPamCounts = words[8];

            for (int i = 0; i < NUM_PRT_WG; i++)
                hot_pkt.wqPrt[i] = words[9 + i];

            hot_pkt.wgPamCounts = words[16];

            hot_pkt.valid = true;
            last_hot_pkt = hot_pkt;
        }

        image::Image<uint16_t> ATMSReader::getChannel(int channel)
        {
            return image::Image<uint16_t>(channels[channel].data(), 96, lines, 1);
        }

        nlohmann::json ATMSReader::getCalib()
        {
            for (int i = 0; i < lines; i++)
                for (int c = 0; c < 22; c++)
                    for (int z = 0; z < 4; z++)
                        calib_data[i]["cold_counts"][c][z] = channels_cc[c][i * 4 + z];
            for (int i = 0; i < lines; i++)
                for (int c = 0; c < 22; c++)
                    for (int z = 0; z < 4; z++)
                        calib_data[i]["warm_counts"][c][z] = channels_wc[c][i * 4 + z];
            return calib_data;
        }
    } // namespace atms
} // namespace jpss
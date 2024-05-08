#include "modis_reader.h"

#include <map>
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"

#include "logger.h"

namespace eos
{
    namespace modis
    {
        MODISReader::MODISReader()
        {
            for (int i = 0; i < 31; i++)
                channels1000m[i].resize(1354 * 10);
            for (int i = 0; i < 5; i++)
                channels500m[i].resize(1354 * 2 * 20);
            for (int i = 0; i < 2; i++)
                channels250m[i].resize(1354 * 4 * 40);
            lines = 0;
            day_count = 0;
            night_count = 0;
        }

        MODISReader::~MODISReader()
        {
            for (int i = 0; i < 31; i++)
                channels1000m[i].clear();
            for (int i = 0; i < 5; i++)
                channels500m[i].clear();
            for (int i = 0; i < 2; i++)
                channels250m[i].clear();
        }

        /*
        The official MODIS User Guide states this is an "exclusive-or checksum",
        but that did not lead to any useful results.

        Looking online, I was however able to find a working and proper
        implementation :
        https://oceancolor.gsfc.nasa.gov/docs/ocssw/check__checksum_8c_source.html

        In the code linked above, the proper specification is :
        -------------------------------------------------
        82                FYI, The checksum is computed as follows:
        83                1: Within each packet, the 12-bit science data (or test data)
        84                   is summed into a 16-bit register, with overflows ignored.
        85                2: After all the data is summed, the 16-bit result is right
        86                   shifted four bits, with zeros inserted into the leading
        87                   four bits.
        88                3: The 12 bits left in the lower end of the word becomes the
        89                   checksum.
        -------------------------------------------------

        The documentation is hence... Not so helpful with this :-)
         */
        uint16_t MODISReader::compute_crc(uint16_t *data, int size)
        {
            uint16_t crc = 0;
            for (int i = 0; i < size; i++)
                crc += data[i];
            crc >>= 4;
            return crc;
        }

        void MODISReader::processDayPacket(ccsds::CCSDSPacket &packet, MODISHeader &header)
        {
            repackBytesTo12bits(&packet.payload[12], 624, modis_ifov);

            // Check CRC
            if (compute_crc(modis_ifov, 415) != modis_ifov[415])
                return;

            // Filter out calibration packets, and process them
            if (header.type_flag == 1)
            {
                for (int i = 0; i < 415; i++)
                {
                    int i_off = (packet.header.sequence_flag == 2 ? 415 : 0) + i;
                    if (header.calib_type == MODISHeader::SOLAR_DIFFUSER_SOURCE)
                        d_calib[lines / 10]["solar_diffuser_source"][header.calib_frame_count][i_off] = modis_ifov[i];
                    else if (header.calib_type == MODISHeader::SRCA_CALIB_SOURCE)
                        d_calib[lines / 10]["srca_diffuser_source"][header.calib_frame_count][i_off] = modis_ifov[i];
                    else if (header.calib_type == MODISHeader::BLACKBODY_SOURCE)
                        d_calib[lines / 10]["blackbody_source"][header.calib_frame_count][i_off] = modis_ifov[i];
                    else if (header.calib_type == MODISHeader::SPACE_SOURCE)
                        d_calib[lines / 10]["space_source"][header.calib_frame_count][i_off] = modis_ifov[i];
                }
                return;
            }

            // Filter out bad packets
            if (header.earth_frame_data_count > 1354 /*|| header.mirror_side > 1*/)
                return;

            // std::cout << (int)packet.header.sequence_flag << " " << (int)header.earth_frame_data_count << std::endl;

            int position = header.earth_frame_data_count - 1;

            if (position == 0 && packet.header.sequence_flag == 1 && lastScanCount != header.scan_count)
            {
                lines += 10;

                for (int i = 0; i < 31; i++)
                    channels1000m[i].resize((lines + 10) * 1354);
                for (int i = 0; i < 5; i++)
                    channels500m[i].resize((lines * 2 + 20) * 1354 * 2);
                for (int i = 0; i < 2; i++)
                    channels250m[i].resize((lines * 4 + 40) * 1354 * 4);

                double timestamp = ccsds::parseCCSDSTimeFull(packet, -4383);
                for (int i = -5; i < 5; i++)
                    timestamps_1000.push_back(timestamp + i * 0.162); // 1000m timestamps
                for (int i = -10; i < 10; i++)
                    timestamps_500.push_back(timestamp + i * 0.081); // 500m timestamps
                for (int i = -20; i < 20; i++)
                    timestamps_250.push_back(timestamp + i * 0.0405); // 250m timestamps
            }

            lastScanCount = header.scan_count;

            if (packet.header.sequence_flag == 1) // Contains IFOVs 1-5
            {
                // Channel 1-2 (250m)
                for (int c = 0; c < 2; c++)
                    for (int i = 0; i < 4; i++)
                        for (int y = 0; y < 4; y++)
                            for (int f = 0; f < 5; f++)
                                channels250m[c][((lines + 5 + f) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = modis_ifov[(4 - f) * 83 + (c * 16) + (i * 4) + y] << 4;

                // Channel 3-7 (500m)
                for (int c = 0; c < 5; c++)
                    for (int i = 0; i < 2; i++)
                        for (int y = 0; y < 2; y++)
                            for (int f = 0; f < 5; f++)
                                channels500m[c][((lines + 5 + f) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = modis_ifov[(4 - f) * 83 + 32 + (c * 4) + (i * 2) + y] << 4;

                // 8-38 1000m channels
                for (int i = 0; i < 31; i++)
                    for (int f = 0; f < 5; f++)
                        channels1000m[i][(lines + 5 + f) * 1354 + position] = modis_ifov[(4 - f) * 83 + 52 + i] << 4;
            }
            else if (packet.header.sequence_flag == 2) // Contains IFOVs 6-10
            {
                // Channel 1-2 (250m)
                for (int c = 0; c < 2; c++)
                    for (int i = 0; i < 4; i++)
                        for (int y = 0; y < 4; y++)
                            for (int f = 0; f < 5; f++)
                                channels250m[c][((lines + f) * 4 + (3 - y)) * (1354 * 4) + position * 4 + i] = modis_ifov[(4 - f) * 83 + (c * 16) + (i * 4) + y] << 4;

                // Channel 3-7 (500m)
                for (int c = 0; c < 5; c++)
                    for (int i = 0; i < 2; i++)
                        for (int y = 0; y < 2; y++)
                            for (int f = 0; f < 5; f++)
                                channels500m[c][((lines + f) * 2 + (1 - y)) * (1354 * 2) + position * 2 + i] = modis_ifov[(4 - f) * 83 + 32 + (c * 4) + (i * 2) + y] << 4;

                // 8-38 1000m channels
                for (int i = 0; i < 31; i++)
                    for (int f = 0; f < 5; f++)
                        channels1000m[i][(lines + f) * 1354 + position] = modis_ifov[(4 - f) * 83 + 52 + i] << 4;
            }

            fillCalib(packet, header);
        }

        void MODISReader::processNightPacket(ccsds::CCSDSPacket &packet, MODISHeader &header)
        {
            repackBytesTo12bits(&packet.payload[12], 258, modis_ifov);

            // Check CRC
            if (compute_crc(modis_ifov, 171) != modis_ifov[171])
                return;

            // Filter out calibration packets. They're always day!
            if (header.type_flag == 1)
                return;

            // Filter out bad packets
            if (header.earth_frame_data_count > 1354 /*|| header.mirror_side > 1*/)
                return;

            // std::cout << (int)packet.header.sequence_flag << " " << (int)header.earth_frame_data_count << std::endl;

            int position = header.earth_frame_data_count - 1;

            if (position == 0 && lastScanCount != header.scan_count)
            {
                lines += 10;

                for (int i = 0; i < 31; i++)
                    channels1000m[i].resize((lines + 10) * 1354);
                for (int i = 0; i < 5; i++)
                    channels500m[i].resize((lines * 2 + 20) * 1354 * 2);
                for (int i = 0; i < 2; i++)
                    channels250m[i].resize((lines * 4 + 40) * 1354 * 4);

                double timestamp = ccsds::parseCCSDSTimeFull(packet, -4383);
                for (int i = -5; i < 5; i++)
                    timestamps_1000.push_back(timestamp + i * 0.162); // 1000m timestamps
                for (int i = -10; i < 10; i++)
                    timestamps_500.push_back(timestamp + i * 0.081); // 500m timestamps
                for (int i = -20; i < 20; i++)
                    timestamps_250.push_back(timestamp + i * 0.0405); // 250m timestamps
            }

            lastScanCount = header.scan_count;

            // 28 1000m channels
            for (int i = 0; i < 17; i++)
                for (int f = 0; f < 10; f++)
                    channels1000m[14 + i][(lines + f) * 1354 + position] = modis_ifov[(9 - f) * 17 + i] << 4;

            fillCalib(packet, header);
        }

        void MODISReader::fillCalib(ccsds::CCSDSPacket &packet, MODISHeader &header)
        {
            d_calib[lines / 10]["night_group"] = header.packet_type == MODISHeader::NIGHT_GROUP;
            d_calib[lines / 10]["mirror_side"] = header.mirror_side;
            for (int i = 0; i < 12; i++)
                d_calib[lines / 10]["bb_temp"][i] = last_obc_bb_temp[i];
            for (int i = 0; i < 2; i++)
                d_calib[lines / 10]["mir_temp"][i] = last_rct_mir_temp[i];
            for (int i = 0; i < 4; i++)
                d_calib[lines / 10]["cav_temp"][i] = last_cav_temp[i];
            for (int i = 0; i < 4; i++)
                d_calib[lines / 10]["inst_temp"][i] = last_ao_inst_temp[i];
            for (int i = 0; i < 4; i++)
                d_calib[lines / 10]["fp_temp"][i] = last_fp_temp[i];
            for (int i = 0; i < 4; i++)
                d_calib[lines / 10]["fp_temp_info"][i] = last_cr_rc_info[i];
        }

        void MODISReader::work(ccsds::CCSDSPacket &packet)
        {
            // Filter out bad packets
            if (packet.payload.size() < 10)
                return;

            MODISHeader modisHeader(packet);

            if (modisHeader.packet_type == MODISHeader::DAY_GROUP)
            {
                if (packet.payload.size() < 636)
                    return;

                day_count++;
                processDayPacket(packet, modisHeader);
            }
            else if (modisHeader.packet_type == MODISHeader::NIGHT_GROUP)
            {
                if (packet.payload.size() < 270)
                    return;

                night_count++;
                processNightPacket(packet, modisHeader);
            }
            else if (modisHeader.packet_type == MODISHeader::ENG_GROUP_1)
            {
                // printf("ENG1!\n");
                if (packet.payload.size() < 636)
                    return;

                processEng1Packet(packet, modisHeader);
            }
            else if (modisHeader.packet_type == MODISHeader::ENG_GROUP_2)
            {
                // printf("ENG2!\n");
                if (packet.payload.size() < 636)
                    return;

                processEng2Packet(packet, modisHeader);
            }
        }

        void MODISReader::processEng1Packet(ccsds::CCSDSPacket &packet, MODISHeader &header)
        {
            // printf("ENG1 %d\n", packet.header.sequence_flag);
            if (packet.header.sequence_flag == 1)
            {
            }
            else if (packet.header.sequence_flag == 2)
            {
                repackBytesTo12bits(&packet.payload[298], 18, last_obc_bb_temp);               // Obc BB Temp
                last_rct_mir_temp[0] = (packet.payload[504] & 0xF) << 8 | packet.payload[505]; // TP_SA_RCT1_MIRE
                last_rct_mir_temp[1] = packet.payload[506] << 4 | packet.payload[507] >> 4;    // TP_SA_RCT2_MIRE

                last_fp_temp[1] = (packet.payload[496] & 0b111111) << 6 | packet.payload[497] >> 2;                         // TA_AO_NIR_FPAE
                last_fp_temp[0] = (packet.payload[497] & 0b11) << 10 | packet.payload[498] << 2 | packet.payload[499] >> 6; // TA_AO_VIS_FPAE
                last_fp_temp[3] = (packet.payload[499] & 0b111111) << 6 | packet.payload[500] >> 2;                         // TA_RC_LWIR_CFPAE
                last_fp_temp[2] = (packet.payload[500] & 0b11) << 10 | packet.payload[501] << 2 | packet.payload[502] >> 6; // TA_RC_SMIR_CFPAE
            }
        }

        void MODISReader::processEng2Packet(ccsds::CCSDSPacket &packet, MODISHeader &header)
        {
            // printf("ENG2 %d\n", packet.header.sequence_flag);
            if (packet.header.sequence_flag == 1)
            {
                auto func_m = [this](uint8_t *block_curr, int major_cycle_cnt)
                {
                    if (major_cycle_cnt == 0)
                    {
                        uint16_t words_tp_ao[5 + 1];
                        repackBytesTo12bits(&block_curr[56], 8, words_tp_ao);
                        last_ao_inst_temp[0] = words_tp_ao[4]; // TP_AO_SMIR_OBJ
                        last_ao_inst_temp[1] = words_tp_ao[1]; // TP_AO_LWIR_OBJ
                        last_ao_inst_temp[2] = words_tp_ao[3]; // TP_AO_SMIR_LENS
                        last_ao_inst_temp[3] = words_tp_ao[0]; // TP_AO_LWIR_LENS
                    }
                    else if (major_cycle_cnt == 3)
                    {
                        last_cav_temp[0] = block_curr[56] << 1 | block_curr[57] >> 7;               // TP_MF_CALBKHD_SR
                        last_cav_temp[3] = (block_curr[57] & 0b1111111) << 2 | block_curr[58] >> 6; // TP_MF_CVR_OP_SR
                    }
                    else if (major_cycle_cnt == 4)
                    {
                        last_cav_temp[2] = (block_curr[58] & 0b111111) << 3 | block_curr[59] >> 5; // TP_MF_Z_BKHD_BB
                    }
                    else if (major_cycle_cnt == 23)
                    {
                        last_cav_temp[1] = (block_curr[58] & 0b111111) << 3 | block_curr[59] >> 5; // TP_SR_SNOUT
                    }

                    if (major_cycle_cnt == 5 ||
                        major_cycle_cnt == 13 ||
                        major_cycle_cnt == 21 ||
                        major_cycle_cnt == 29 ||
                        major_cycle_cnt == 37 ||
                        major_cycle_cnt == 45 ||
                        major_cycle_cnt == 53 ||
                        major_cycle_cnt == 61)
                    {
                        last_cr_rc_info[0] = block_curr[43] >> 7;       // CR_RC_CFPA_T1SET
                        last_cr_rc_info[1] = (block_curr[43] >> 6) & 1; // CR_RC_CFPA_T3SET
                        last_cr_rc_info[2] = (block_curr[43] >> 2) & 1; // CR_RC_LWHTR_ON
                        last_cr_rc_info[3] = (block_curr[44] >> 6) & 1; // CR_RC_SMHTR_ON
                    }
                };

                uint8_t *block_curr1 = &packet.payload[12];
                uint8_t *block_curr2 = &packet.payload[12 + 64];
                int major_cycle_cnt1 = block_curr1[0] >> 2;
                int major_cycle_cnt2 = block_curr2[0] >> 2;

                func_m(block_curr2, major_cycle_cnt2); // Prior
                func_m(block_curr1, major_cycle_cnt1); // Current

                // logger->critical("Major Cycle %d - %d", major_cycle_cnt1, major_cycle_cnt2);
            }
            else if (packet.header.sequence_flag == 2)
            {
            }
        }

        image::Image MODISReader::getImage250m(int channel)
        {
            return image::Image(channels250m[channel].data(), 16, 1354 * 4, lines * 4, 1);
        }

        image::Image MODISReader::getImage500m(int channel)
        {
            return image::Image(channels500m[channel].data(), 16, 1354 * 2, lines * 2, 1);
        }

        image::Image MODISReader::getImage1000m(int channel)
        {
            return image::Image(channels1000m[channel].data(), 16, 1354, lines, 1);
        }

        nlohmann::json MODISReader::getCalib()
        {
            return d_calib;
        }
    } // namespace modis
} // namespace eos
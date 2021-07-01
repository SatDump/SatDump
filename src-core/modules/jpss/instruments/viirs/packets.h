#pragma once

#include <cstdint>
#include <cmath>
#include <cstring>
#include "common/ccsds/ccsds.h"
#include "channels.h"

namespace jpss
{
    namespace viirs
    {
        // VIIRS Segment Header packet
        struct HeaderPacket : public ccsds::CCSDSPacket
        {
            HeaderPacket(ccsds::CCSDSPacket &packet)
            {
                header = packet.header;
                payload = packet.payload;

                time_start_of_scan = (uint64_t)payload[0] << 56 |
                                     (uint64_t)payload[1] << 48 |
                                     (uint64_t)payload[2] << 40 |
                                     (uint64_t)payload[3] << 32 |
                                     (uint64_t)payload[4] << 24 |
                                     (uint64_t)payload[5] << 16 |
                                     (uint64_t)payload[6] << 8 |
                                     (uint64_t)payload[7];

                number_of_packets = payload[8];

                viirs_sequence_count = payload[9] << 24 |
                                       payload[10] << 16 |
                                       payload[11] << 8 |
                                       payload[12];

                packet_time = (uint64_t)payload[13] << 56 |
                              (uint64_t)payload[14] << 48 |
                              (uint64_t)payload[15] << 40 |
                              (uint64_t)payload[16] << 32 |
                              (uint64_t)payload[17] << 24 |
                              (uint64_t)payload[18] << 16 |
                              (uint64_t)payload[19] << 8 |
                              (uint64_t)payload[20];

                format_version = payload[21];
                instrument_number = payload[22];

                ham_side = payload[24] >> 7;
                scan_synch = (payload[25] >> 6) % 2;
                self_test_data_pattern = (payload[26] >> 2) % (int)pow(2, 4);

                scan_number = payload[27] << 24 |
                              payload[28] << 16 |
                              payload[29] << 8 |
                              payload[30];

                scan_terminus = (uint64_t)payload[31] << 56 |
                                (uint64_t)payload[32] << 48 |
                                (uint64_t)payload[33] << 40 |
                                (uint64_t)payload[34] << 32 |
                                (uint64_t)payload[35] << 24 |
                                (uint64_t)payload[36] << 16 |
                                (uint64_t)payload[37] << 8 |
                                (uint64_t)payload[38];

                sensor_mode = payload[39];
                viirs_model = payload[40];
                fsw_version = payload[41] << 8 | payload[40];

                band_control_word = payload[42] << 24 |
                                    payload[43] << 16 |
                                    payload[44] << 8 |
                                    payload[45];

                partial_start = payload[46] << 8 | payload[47];

                number_of_samples = payload[48] << 8 | payload[49];

                sample_delay = payload[50] << 8 | payload[51];
            }

            uint64_t time_start_of_scan;
            uint8_t number_of_packets;
            uint32_t viirs_sequence_count;
            uint64_t packet_time;
            uint8_t format_version;
            uint8_t instrument_number;

            bool ham_side;
            bool scan_synch;
            uint8_t self_test_data_pattern;
            uint32_t scan_number;
            uint64_t scan_terminus;
            uint8_t sensor_mode;
            uint8_t viirs_model;
            uint16_t fsw_version;
            uint32_t band_control_word;
            uint16_t partial_start;
            uint16_t number_of_samples;
            uint16_t sample_delay;
        };

        // VIIRS Detector
        struct Detector
        {
            // Based on weatherdump
            void bitSlicer(int &len, int fillsize)
            {
                int bits = 0, bytes = 0;

                while (fillsize % 8 != 0)
                {
                    bits++;
                    fillsize--;
                }

                bytes = len - (fillsize / 8);

                if (bytes > len || bytes < 0)
                    return;

                len = bytes + 1;
            }

            Detector()
            {
            }

            Detector(uint8_t *data, int size)
            {
                if (size < 88)
                    return;

                fill_data = data[0];
                checksum_offset = data[2] << 8 | data[3];

                if (checksum_offset < 4 || checksum_offset >= size + 4)
                    return;

                data_payload_size = (checksum_offset - 4);

                if (data_payload_size <= 0)
                    return;

                data_payload = new uint8_t[data_payload_size];

                std::memcpy(data_payload, &data[4], data_payload_size);

                bitSlicer(data_payload_size, fill_data);

                if (size - checksum_offset > 8)
                {
                    checksum = data[checksum_offset + 0] << 24 |
                               data[checksum_offset + 1] << 16 |
                               data[checksum_offset + 2] << 8 |
                               data[checksum_offset + 3];
                    sync_word = data[checksum_offset + 4] << 24 |
                                data[checksum_offset + 5] << 16 |
                                data[checksum_offset + 6] << 8 |
                                data[checksum_offset + 7];
                }
                else
                {
                    sync_word = 0xC000FFEE;
                }
            }

            void initDataPayload(int width, int oversample)
            {
                decompressedPayload = new uint16_t[width * oversample];
            }

            uint16_t fill_data;
            uint16_t checksum_offset;
            uint32_t checksum;
            uint32_t sync_word;
            int data_payload_size;
            uint8_t *data_payload;
            uint16_t *decompressedPayload;
        };

        // VIIRS Body / End packet
        struct BodyPacket : public ccsds::CCSDSPacket
        {
            bool empty;

            BodyPacket()
            {
                empty = true;
            };

            BodyPacket(ccsds::CCSDSPacket &packet)
            {
                if (packet.payload.size() < 88)
                    return;

                empty = false;

                header = packet.header;
                payload = packet.payload;

                viirs_sequence_count = payload[0] << 24 |
                                       payload[1] << 16 |
                                       payload[2] << 8 |
                                       payload[3];

                packet_time = (uint64_t)payload[4] << 56 |
                              (uint64_t)payload[5] << 48 |
                              (uint64_t)payload[6] << 40 |
                              (uint64_t)payload[7] << 32 |
                              (uint64_t)payload[8] << 24 |
                              (uint64_t)payload[9] << 16 |
                              (uint64_t)payload[10] << 8 |
                              (uint64_t)payload[11];

                format_version = payload[12];
                instrument_number = payload[13];

                integrity_check = payload[16] >> 7;
                self_test_data_pattern = (payload[16] >> 2) % (int)pow(2, 4);

                band = payload[18];
                detector = payload[19];
                sync_word_pattern = payload[20] << 24 |
                                    payload[21] << 16 |
                                    payload[22] << 8 |
                                    payload[23];

                int offset = 88;
                for (int i = 0; i < 6; i++)
                {
                    if (offset < (int)payload.size())
                    {
                        detectors[i] = Detector(&payload[offset], payload.size() - offset);
                        offset += detectors[i].checksum_offset + 8;
                    }
                }
            }

            uint32_t viirs_sequence_count;
            uint64_t packet_time;
            uint8_t format_version;
            uint8_t instrument_number;

            bool integrity_check;
            uint8_t self_test_data_pattern;
            uint8_t band;
            uint8_t detector;
            uint32_t sync_word_pattern;

            Detector detectors[6];
        };

        // A segment
        struct Segment
        {
            Segment(HeaderPacket &pktheader) : header(pktheader)
            {
            }

            HeaderPacket header;
            BodyPacket body[32];
        };
    } // namespace viirs
} // namespace jpss
#include "amsu_reader.h"

#include "common/ccsds/ccsds_time.h"

namespace noaa_metop
{
    namespace amsu
    {
        AMSUReader::AMSUReader()
        {
            for (int i = 0; i < 15; i++)
                channels[i].resize(30);
        }

        AMSUReader::~AMSUReader()
        {
            for (int i = 0; i < 15; i++)
                channels[i].clear();
            timestamps_A1.clear();
            timestamps_A2.clear();
        }

        void AMSUReader::work_A1(uint8_t *buffer)
        {
            for (int n = 2; n < 15; n++)
                channels[n].resize(channels[n].size() + 30);
            for (int i = 0; i < 1020; i += 34)
            {
                for (int j = 0; j < 13; j++)
                {
                    channels[j + 2][30 * linesA1 + i / 34] = (buffer[i + 16 + 2 * j] << 8) | buffer[16 + i + 2 * j + 1];
                }
            }
            linesA1++;
        }

        void AMSUReader::work_A2(uint8_t *buffer)
        {
            for (int n = 0; n < 2; n++)
                channels[n].resize(channels[n].size() + 30);

            for (int i = 0; i < 240; i += 8)
            {
                channels[0][30 * linesA2 + i / 8] = buffer[i + 12] << 8 | buffer[12 + i + 1];
                channels[1][30 * linesA2 + i / 8] = buffer[i + 12 + 2] << 8 | buffer[12 + i + 1 + 2];
            }

            linesA2++;
        }

        void AMSUReader::work_noaa(uint8_t *buffer)
        {
            uint8_t lines_since_timestamp = buffer[5] & 3;

            std::vector<uint8_t> amsuA2words;
            std::vector<uint8_t> amsuA1words;

            for (int j = 0; j < 14; j += 2)
            {
                if ((buffer[j + 35] % 2 != 1) || (buffer[j + 34] == 0xFF) || (buffer[j + 35] == 0xFF))
                {
                    amsuA2words.push_back(buffer[j + 34]);
                    amsuA2words.push_back(buffer[j + 35]);
                }
            }
            for (int j = 0; j < 26; j += 2)
            {
                if ((buffer[j + 9] % 2 != 1) || (buffer[j + 8] == 0xFF) || (buffer[j + 9] == 0xFF))
                {
                    amsuA1words.push_back(buffer[j + 8]);
                    amsuA1words.push_back(buffer[j + 9]);
                }
            }

            std::vector<std::vector<uint8_t>> amsuA2Data = amsuA2Deframer.work(amsuA2words.data(), amsuA2words.size());
            std::vector<std::vector<uint8_t>> amsuA1Data = amsuA1Deframer.work(amsuA1words.data(), amsuA1words.size());

            for (std::vector<uint8_t> frame : amsuA2Data)
            {
                work_A2(frame.data());
                if (contains(timestamps_A2, last_TIP_timestamp + (last_TIP_timestamp != -1 ? 8 * lines_since_timestamp : 0)))
                    timestamps_A2.push_back(-1);
                else
                    timestamps_A2.push_back(last_TIP_timestamp + (last_TIP_timestamp != -1 ? 8 * lines_since_timestamp : 0));
            }

            for (std::vector<uint8_t> frame : amsuA1Data)
            {
                work_A1(frame.data());
                if (contains(timestamps_A1, last_TIP_timestamp + (last_TIP_timestamp != -1 ? 8 * lines_since_timestamp : 0)))
                    timestamps_A1.push_back(-1);
                else
                    timestamps_A1.push_back(last_TIP_timestamp + (last_TIP_timestamp != -1 ? 8 * lines_since_timestamp : 0));
            }
        }

        void AMSUReader::work_metop(ccsds::CCSDSPacket &packet)
        {
            if (packet.header.apid == 39)
            {
                if (packet.payload.size() < 2096)
                    return;
                std::vector<uint8_t> filtered;
                for (unsigned int i = 13; i < packet.payload.size(); i += 2)
                {
                    uint16_t word = (packet.payload[i + 1] << 8) | packet.payload[i + 2];
                    if (word != 1){
                        filtered.push_back(word >> 8);
                        filtered.push_back(word & 0xFF);
                    }
                }
                work_A1(filtered.data());
                timestamps_A1.push_back(ccsds::parseCCSDSTimeFull(packet, 10957));
            }
            else if (packet.header.apid == 40)
            {
                if (packet.payload.size() < 1136)
                    return;
                std::vector<uint8_t> filtered;
                for (unsigned int i = 13; i < packet.payload.size(); i += 2)
                {
                    uint16_t word = (packet.payload[i + 1] << 8) | packet.payload[i + 2];
                    if (word != 1){
                        filtered.push_back(word >> 8);
                        filtered.push_back(word & 0xFF);
                    }
                }
                work_A2(filtered.data());
                timestamps_A2.push_back(ccsds::parseCCSDSTimeFull(packet, 10957));
            }
        }
    }
}
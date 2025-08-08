#include "ccsds.h"
#include <cmath>
#include <cstdint>
#include <cstring>

#include <cstdio>

namespace ccsds
{
    CCSDSHeader::CCSDSHeader() {}

    CCSDSHeader::CCSDSHeader(uint8_t *rawi)
    {
        std::memcpy(raw, rawi, 6);
        version = rawi[0] >> 5;
        type = (rawi[0] >> 4) % 2;
        secondary_header_flag = (rawi[0] >> 3) % 2;
        apid = (rawi[0] % (int)pow(2, 3)) << 8 | rawi[1];
        sequence_flag = rawi[2] >> 6;
        packet_sequence_count = (rawi[2] % (int)pow(2, 6)) << 8 | rawi[3];
        packet_length = rawi[4] << 8 | rawi[5];
    }

    void CCSDSHeader::encodeHDR()
    {
        raw[0] = (version & 0b111) << 5 | (type & 0b1) << 4 | (secondary_header_flag & 0b1) << 3 | ((apid >> 8) & 0b111);
        raw[1] = apid & 0xFF;
        raw[2] = (sequence_flag & 0b11) << 6 | ((packet_sequence_count >> 8) & 0b111111);
        raw[3] = packet_sequence_count & 0xFF;
        raw[4] = packet_length >> 8;
        raw[5] = packet_length & 0xFF;
    }

    void CCSDSPacket::encodeHDR()
    {
        header.packet_length = payload.size() - 1;
        header.encodeHDR();
    }

    // Parse CCSDS header
    CCSDSHeader parseCCSDSHeader(uint8_t *header) { return CCSDSHeader(header); }

    CCSDSHeader::CCSDSHeader(const CCSDSHeader &v) noexcept
    {
        memcpy(raw, v.raw, 6);
        version = v.version;
        type = v.type;
        secondary_header_flag = v.secondary_header_flag;
        apid = v.apid;
        sequence_flag = v.sequence_flag;
        packet_sequence_count = v.packet_sequence_count;
        packet_length = v.packet_length;
    }

    CCSDSHeader &CCSDSHeader::operator=(const CCSDSHeader &v) noexcept
    {
        if (this != &v)
        {
            memcpy(raw, v.raw, 6);
            version = v.version;
            type = v.type;
            secondary_header_flag = v.secondary_header_flag;
            apid = v.apid;
            sequence_flag = v.sequence_flag;
            packet_sequence_count = v.packet_sequence_count;
            packet_length = v.packet_length;
        }
        return *this;
    };

    CCSDSPacket::CCSDSPacket(const CCSDSPacket &v) noexcept
    {
        header = v.header;
        payload = v.payload;
    }

    CCSDSPacket &CCSDSPacket::operator=(const CCSDSPacket &v) noexcept
    {
        if (this != &v)
        {
            header = v.header;
            payload = v.payload;
        }
        return *this;
    };

    bool crcCheckCCITT(CCSDSPacket &pkt)
    {
        uint16_t crc2 = pkt.payload[pkt.payload.size() - 2] << 8 | pkt.payload[pkt.payload.size() - 1];
        uint16_t crc = 0xFFFF;

        const int CCITT_CRC_GEN = 0x1021;
        for (int j = 0; j < (int)pkt.payload.size() + 6 - 2; j++)
        {
            uint8_t val = j < 6 ? pkt.header.raw[j] : pkt.payload[j - 6];

            unsigned short dataByte = static_cast<unsigned short>(val) << 8;
            for (int i = 8; i > 0; i--)
            {
                if ((dataByte ^ crc) & 0x8000)
                    crc = (crc << 1) ^ CCITT_CRC_GEN;
                else
                    crc <<= 1;
                dataByte <<= 1;
            }
        }

        return crc == crc2;
    }

    bool crcCheckHLDC32(CCSDSPacket &pkt)
    {
        uint32_t crc2 = pkt.payload[pkt.payload.size() - 4] << 24 | pkt.payload[pkt.payload.size() - 3] << 16 | pkt.payload[pkt.payload.size() - 2] << 8 | pkt.payload[pkt.payload.size() - 1];
        uint32_t crc = 0xFFFFFFFF;

        const int HDLC_CRC_GEN = 0xEDB88320;
        for (int j = 0; j < (int)pkt.payload.size() + 6 - 4; j++)
        {
            uint8_t val = j < 6 ? pkt.header.raw[j] : pkt.payload[j - 6];

            unsigned short dataByte = static_cast<unsigned short>(val) << 8;
            for (int i = 8; i > 0; i--)
            {
                if ((dataByte ^ crc) & 0x80000000)
                    crc = (crc << 1) ^ HDLC_CRC_GEN;
                else
                    crc <<= 1;
                dataByte <<= 1;
            }
        }

        return crc ^ 0xFFFFFFFF == crc2;
    }

    bool crcCheckVerticalParity(CCSDSPacket &pkt)
    {
        uint16_t crc2 = pkt.payload[pkt.payload.size() - 2] << 8 | pkt.payload[pkt.payload.size() - 1];

        uint16_t checksum = 0;
        for (int j = 0; j < (int)(pkt.payload.size() + 6 - 2) / 2; j++)
        {
            int j1 = j * 2 + 0;
            int j2 = j * 2 + 1;
            uint8_t val1 = j1 < 6 ? pkt.header.raw[j1] : pkt.payload[j1 - 6];
            uint8_t val2 = j2 < 6 ? pkt.header.raw[j2] : pkt.payload[j2 - 6];
            checksum ^= val1 << 8 | val2;
        }

        return crc2 == checksum;
    }
} // namespace ccsds
#pragma once

/**
 * @file  ccsds.h
 * @brief Space Packet definition
 */

#include <cstdint>
#include <vector>

namespace ccsds
{
    /**
     * @brief Struct representing a CCSDS Space Packet header
     */
    struct CCSDSHeader
    {
        uint8_t raw[6];
        uint8_t version;
        bool type;
        bool secondary_header_flag;
        uint16_t apid;
        uint8_t sequence_flag;
        uint16_t packet_sequence_count;
        uint16_t packet_length;

        CCSDSHeader();
        CCSDSHeader(uint8_t *rawi);
        CCSDSHeader(const CCSDSHeader &v) noexcept;
        void encodeHDR();

        CCSDSHeader &operator=(const CCSDSHeader &v) noexcept;
    };

    /**
     * @brief Struct representing a CCSDS Space Packet
     */
    struct CCSDSPacket
    {
        CCSDSHeader header;
        std::vector<uint8_t> payload;
        void encodeHDR();

        CCSDSPacket() {}
        CCSDSPacket(const CCSDSPacket &v) noexcept;

        CCSDSPacket &operator=(const CCSDSPacket &v) noexcept;
    };

    /**
     * @brief Parse a CCSDS Header
     *
     * @param header pointer to a CCSDS header in a buffer
     */
    CCSDSHeader parseCCSDSHeader(uint8_t *header);

    /**
     * @brief Check a CCSDS Packet's CCITT CRC (if present)
     *
     * @param pkt Space Packet
     * @return true if valid
     */
    bool crcCheckCCITT(CCSDSPacket &pkt);

    /**
     * @brief Check a CCSDS Packet's Verical Parity (if present)
     *
     * @param pkt Space Packet
     * @return true if valid
     */
    bool crcCheckVerticalParity(CCSDSPacket &pkt);
} // namespace ccsds
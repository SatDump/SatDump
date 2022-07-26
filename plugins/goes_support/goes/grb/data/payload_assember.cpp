#include "payload_assembler.h"
#include "logger.h"

namespace goes
{
    namespace grb
    {
        GRBFilePayloadAssembler::GRBFilePayloadAssembler()
        {
        }

        void GRBFilePayloadAssembler::work(ccsds::CCSDSPacket &pkt)
        {
            // If lengths do not match, discard
            if (pkt.header.packet_length + 1 != (int)pkt.payload.size())
                return;

            if (current_payloads.count(pkt.header.apid) == 0)
                current_payloads.insert({pkt.header.apid, GRBFilePayload()});

            GRBFilePayload &current_payload = current_payloads[pkt.header.apid];

            if (pkt.header.sequence_flag == 1 || pkt.header.sequence_flag == 3)
            {
                if (current_payload.in_progress)
                {
                    // Process
                    if (current_payload.valid)
                        processor->processPayload(current_payload);
                }

                // Reset
                current_payload = GRBFilePayload();

                // Check CRC. We need to recompose the packet for that
                bool crc_ok = crc_valid(pkt);
                if (!crc_ok && !ignore_crc)
                {
                    logger->error("Invalid CRC. Discarding payload.");
                    return;
                }

                // Parse secondary header
                current_payload.apid = pkt.header.apid;
                current_payload.sec_header = GRBSecondaryHeader(&pkt.payload.data()[0]);

                // Fill in payload, discarding CRC and header
                current_payload.payload.insert(current_payload.payload.end(), &pkt.payload.data()[8], &pkt.payload.data()[pkt.payload.size() - 4]);

                current_payload.in_progress = true;
            }
            else if (pkt.header.sequence_flag == 0 || pkt.header.sequence_flag == 2)
            {
                bool crc_ok = crc_valid(pkt);
                if (!crc_ok && !ignore_crc)
                {
                    current_payload.in_progress = false;
                    current_payload.valid = false;
                    logger->error("Invalid CRC. Discarding payload.");
                    return;
                }

                if (current_payload.in_progress &&          // Make sure a file is in progress
                    current_payload.apid == pkt.header.apid // ....and that the APID matches
                )
                {
                    current_payload.payload.insert(current_payload.payload.end(), &pkt.payload.data()[8], &pkt.payload.data()[pkt.payload.size() - 4]);
                }

                if (pkt.header.sequence_flag == 2 && current_payload.in_progress) // We're done with this payload
                {
                    // Process
                    if (current_payload.valid)
                        processor->processPayload(current_payload);
                    current_payload.in_progress = false;
                }
            }
        }

        bool GRBFilePayloadAssembler::crc_valid(ccsds::CCSDSPacket &pkt)
        {
            // Extract CRC
            uint32_t sent_checksum = pkt.payload[pkt.payload.size() - 4] << 24 |
                                     pkt.payload[pkt.payload.size() - 3] << 16 |
                                     pkt.payload[pkt.payload.size() - 2] << 8 |
                                     pkt.payload[pkt.payload.size() - 1];

            // We need to re-compose the packet
            std::vector<uint8_t> fullPkt;

            try
            {
                fullPkt.insert(fullPkt.end(), &pkt.header.raw[0], &pkt.header.raw[6]);
                fullPkt.insert(fullPkt.end(), &pkt.payload[0], &pkt.payload[pkt.payload.size() - 4]);
            }
            catch (std::exception &e)
            {
                return false;
            }

            // Compute out own
            uint32_t checksum = crc.compute(fullPkt.data(), fullPkt.size());
            fullPkt.clear(); // Free memory

            // if (checksum != sent_checksum)
            //     logger->critical("CRC CHECK {:d} {:d}", sent_checksum, checksum);

            return checksum == sent_checksum;
        }
    }
}
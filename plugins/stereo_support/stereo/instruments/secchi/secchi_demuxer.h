#pragma once

#include "common/ccsds/ccsds.h"

#include "logger.h"

namespace stereo
{
    namespace secchi
    {
        struct block_pkt_hdr_t
        {
            uint16_t image_count;  // 0x7FF is fill
            uint8_t block_type;    // 0 data, 1 header, 2 trailer
            uint16_t block_number; // 0 to 16383
            uint16_t block_length; // Block length (+1 is done when parsing!)
        };

        struct SECCHIBlock
        {
            block_pkt_hdr_t hdr;
            std::vector<uint8_t> payload;
        };

        // Must be at least 6 bytes long!
        inline block_pkt_hdr_t parse_block_pkt_hdr(uint8_t *data)
        {
            block_pkt_hdr_t v;
            v.image_count = (data[0] & 0b111) << 8 | data[1];
            v.block_type = data[2] >> 6;
            v.block_number = (data[2] & 0b111111) << 8 | data[3];
            v.block_length = (data[4] << 8 | data[5]) + 1;
            return v;
        }

        class PayloadAssembler
        {
        private:
            std::vector<uint8_t> wip_block;

        public:
            std::vector<SECCHIBlock> work(ccsds::CCSDSPacket &pkt)
            {
                std::vector<SECCHIBlock> blocks;

                uint16_t block_pointer = pkt.payload[6] << 8 | pkt.payload[7];

                if (block_pointer >= 258 && block_pointer != 0x7FF)
                    return blocks;

                // logger->trace("Block Ptr %d", block_pointer);

                if (block_pointer == 0x7FF) // No new header. Append!
                {
                    wip_block.insert(wip_block.end(), &pkt.payload[8], &pkt.payload[8 + 258]);
                }
                else
                {
                    wip_block.insert(wip_block.end(), &pkt.payload[8], &pkt.payload[8 + block_pointer]);
                    process_block(blocks);
                    wip_block.clear();

                    wip_block.insert(wip_block.end(), &pkt.payload[8 + block_pointer], &pkt.payload[8 + 258]);

                    // We can have another starting!
                    if (wip_block.size() >= 6)
                    {
                        block_pkt_hdr_t hdr = parse_block_pkt_hdr(wip_block.data());

                        if (wip_block.size() - 6 >= hdr.block_length)
                        {
                            process_block(blocks);
                            wip_block.clear();

                            wip_block.insert(wip_block.end(), &pkt.payload[8 + block_pointer + hdr.block_length + 6], &pkt.payload[8 + 258]);
                        }
                    }
                }

                return blocks;
            }

            void process_block(std::vector<SECCHIBlock> &blocks)
            {
                if (wip_block.size() < 6)
                    return;

                block_pkt_hdr_t hdr = parse_block_pkt_hdr(wip_block.data());

                if (hdr.image_count == 0x7FF)
                    return;

                // logger->info("%d %d %d %d - %d", hdr.image_count, hdr.block_type, hdr.block_number, hdr.block_length, wip_block.size() - 6);

                if (wip_block.size() - 6 >= hdr.block_length)
                {
                    wip_block.erase(wip_block.begin(), wip_block.begin() + 6);
                    blocks.push_back({hdr, wip_block});
                }
            }
        };
    }
}
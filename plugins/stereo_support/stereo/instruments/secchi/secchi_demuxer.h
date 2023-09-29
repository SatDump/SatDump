#pragma once

#include "common/ccsds/ccsds.h"

#include <cstring>
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

    struct BaseHeader
    {
        /** Processed image output filename  */
        char filename[13];

        /** headers present ( 0-base, 1-base+nominal, 2-all 3 )
          Determined by IP logic */
        uint8_t eHeaderTypeFlag;
        uint8_t version;

        /** image sequence counter e.g. nth in sequence */
        uint8_t imgSeq;

        /** Total num in sequence */
        uint8_t numInSeq;

        /** on chip CCD binning as specified by camera readout sequence */
        uint8_t sumrow;
        uint8_t sumcol;

        /** actual Filter wheel position */
        uint8_t actualFilterPosition;

        /** actual Polarization wheel or Quad position */
        uint8_t actualPolarPosition;

        /** actual Polarization wheel position 2*/
        uint8_t actualPolarPosition2;

        /** image processing binning as determined by IP task */
        uint8_t sebxsum;
        uint8_t sebysum;

        /** IP processing log of commands */
        uint8_t ipCmdLog[20];

        /** Ground Sofware usage */
        uint16_t osNumber;

        /** Critical Event Code - alert SECCHI OPs */
        uint16_t critEvent;

        /** image counter - added by IS [0-9999] */
        uint16_t imgCtr;

        /** Telescope image counter (same counter as HK packet ) */
        uint16_t telescopeImgCnt;

        /** CCD mean and stdev */
        int32_t meanBias;
        uint32_t stddevBias;

        /** Actual exp duration 24 bits with LSB = 4 usec
        ( Close time minus Open time )
         For HI the time is measured by FSW using a
         hardware timer and converted to 4 usec units. */
        uint32_t actualExpDuration;

        /** Actual for 2nd exposure duration ( 24 bits with LSB = 4 usec )
        ( Close time minus Open time ) source: mechanism readback
         For HI the time is measured by FSW using a
         hardware timer and converted to 4 usec units. */
        uint32_t actualExpDuration_2;

        /** time (UTC) of actual exposure (Shutter command) NOT including
        lt_offset */
#ifdef LINUX
        uint32 spare;
#endif
        double actualExpTime;

        /** time (UTC) of actual exposure #2 (Shutter command) NOT including
        lt_offset */
        double actualExpTime_2;
    };

    inline BaseHeader read_base_hdr(uint8_t *buf)
    {
        BaseHeader hdr;

        memcpy(hdr.filename, &buf[0], 13);

        hdr.eHeaderTypeFlag = buf[13];
        hdr.version = buf[14];

        hdr.imgSeq = buf[15];
        hdr.numInSeq = buf[16];

        hdr.sumrow = buf[17];
        hdr.sumcol = buf[18];

        hdr.actualFilterPosition = buf[19];
        hdr.actualPolarPosition = buf[20];
        hdr.actualPolarPosition2 = buf[21];

        hdr.sebxsum = buf[22];
        hdr.sebysum = buf[23];

        memcpy(hdr.ipCmdLog, &buf[24], 20);

        hdr.osNumber = buf[44] << 8 | buf[45];

        hdr.critEvent = buf[46] << 8 | buf[47];

        hdr.imgCtr = buf[48] << 8 | buf[49];

        hdr.telescopeImgCnt = buf[50] << 8 | buf[51];

        // 4 + 4 + 4 + 4

        uint8_t *exp_p = (uint8_t *)&hdr.actualExpTime;
        int off = 72;

        exp_p[0 + 0] = buf[off + 7];
        exp_p[0 + 1] = buf[off + 6];
        exp_p[0 + 2] = buf[off + 5];
        exp_p[0 + 3] = buf[off + 4];
        exp_p[0 + 4] = buf[off + 3];
        exp_p[0 + 5] = buf[off + 2];
        exp_p[0 + 6] = buf[off + 1];
        exp_p[0 + 7] = buf[off + 0];

        hdr.actualExpTime -= 378691200;

        // hdr.critEvent = buf[16] << 8 | buf[17];

        // hdr.imgCtr = buf[19] << 8 | buf[18];

        // hdr.numInSeq = buf[23] << 8 | buf[22];
        // hdr.telescopeImgCnt = buf[25] << 8 | buf[24];
        // hdr.sumrow = buf[27] << 8 | buf[26];
        // hdr.sumcol = buf[29] << 8 | buf[28];

        // hdr.cmdExpDuration = buf[31] << 8 | buf[30];
        // hdr.cmdExpDuration_2 = buf[33] << 8 | buf[32];

        // hdr.actualFilterPosition = buf[35] << 8 | buf[34];

        return hdr;
    }
}
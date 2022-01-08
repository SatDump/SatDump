#pragma once

#include <vector>
#include "grb_headers.h"
#include "common/ccsds/ccsds.h"
#include "crc32.h"
#include "data_processor.h"

namespace goes
{
    namespace grb
    {
        struct GRBFilePayload
        {
            bool valid = true;
            int apid;
            GRBSecondaryHeader sec_header;
            std::vector<uint8_t> payload;
        };

        class GRBFilePayloadAssembler
        {
        private:
            const std::string directory;
            bool file_in_progress = false;
            GRBFilePayload current_payload;
            GRBDataProcessor processor;
            CRC32 crc;

        private:
            bool crc_valid(ccsds::CCSDSPacket &pkt);

        public:
            GRBFilePayloadAssembler(std::string directory);
            void work(ccsds::CCSDSPacket &pkt);
        };
    }
}
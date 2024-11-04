#pragma once

#include <vector>
#include "grb_headers.h"
#include "common/ccsds/ccsds.h"
#include "goes/crc32.h"
#include "data_processor.h"

namespace goes
{
    namespace grb
    {
        struct GRBFilePayload
        {
            bool valid = true;
            bool in_progress = false;
            int apid = 0;
            GRBSecondaryHeader sec_header;
            std::vector<uint8_t> payload;
        };

        class GRBFilePayloadAssembler
        {
        private:
            std::map<int, GRBFilePayload> current_payloads;
            CRC32 crc;

        private:
            bool crc_valid(ccsds::CCSDSPacket &pkt);

        public:
            GRBFilePayloadAssembler();
            void work(ccsds::CCSDSPacket &pkt);

            std::shared_ptr<GRBDataProcessor> processor;

            bool ignore_crc = false;
        };
    }
}
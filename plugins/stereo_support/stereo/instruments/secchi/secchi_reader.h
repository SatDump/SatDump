#pragma once

#include "common/ccsds/ccsds.h"
#include "secchi_demuxer.h"
#include "common/image/image.h"
#include <string>

namespace stereo
{
    namespace secchi
    {
        class SECCHIReader
        {
        private:
            std::string icer_path;
            std::string output_directory;

            secchi::PayloadAssembler secchi_assembler0;
            secchi::PayloadAssembler secchi_assembler1;
            secchi::PayloadAssembler secchi_assembler2;
            secchi::PayloadAssembler secchi_assembler3;

            double last_timestamp_0 = 0;
            double last_timestamp_1 = 0;
            double last_timestamp_2 = 0;
            double last_timestamp_3 = 0;

            double last_polarization_3 = 0;

            int unknown_cnt = 0;

        private:
            image::Image<uint16_t> decompress_icer_tool(uint8_t *data, int dsize, int size);

        public:
            SECCHIReader(std::string icer_path, std::string output_directory);
            void work(ccsds::CCSDSPacket &pkt);
        };
    }
}
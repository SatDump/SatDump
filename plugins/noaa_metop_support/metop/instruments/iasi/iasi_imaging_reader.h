#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image2/image.h"

namespace metop
{
    namespace iasi
    {
        class IASIIMGReader
        {
        private:
            uint16_t iasi_buffer[64 * 64];

        public:
            int lines;
            std::vector<uint16_t> ir_channel;
            std::vector<double> timestamps_ifov;

        public:
            IASIIMGReader();
            ~IASIIMGReader();
            void work(ccsds::CCSDSPacket &packet);
            image2::Image getIRChannel();
        };
    } // namespace iasi
} // namespace metop
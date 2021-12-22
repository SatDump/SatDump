#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image/image.h"
#include "common/resizeable_buffer.h"

namespace metop
{
    namespace iasi
    {
        class IASIIMGReader
        {
        private:
            ResizeableBuffer<unsigned short> ir_channel;
            uint16_t iasi_buffer[64 * 64];

        public:
            IASIIMGReader();
            ~IASIIMGReader();
            int lines;
            std::vector<std::vector<double>> timestamps_ifov;
            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint16_t> getIRChannel();
        };
    } // namespace iasi
} // namespace metop
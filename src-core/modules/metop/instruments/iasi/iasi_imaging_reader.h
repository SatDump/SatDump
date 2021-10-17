#pragma once

#include "common/ccsds/ccsds.h"

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
#include "common/resizeable_buffer.h"

namespace metop
{
    namespace iasi
    {
        class IASIIMGReader
        {
        private:
            ResizeableBuffer<unsigned short> ir_channel;

        public:
            IASIIMGReader();
            ~IASIIMGReader();
            int lines;
            std::vector<std::vector<double>> timestamps_ifov;
            void work(ccsds::CCSDSPacket &packet);
            cimg_library::CImg<unsigned short> getIRChannel();
        };
    } // namespace iasi
} // namespace metop
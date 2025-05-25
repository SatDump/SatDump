#pragma once

#include "common/ccsds/ccsds.h"
#include "image/image.h"
#include "nlohmann/json.hpp"

namespace metop
{
    namespace iasi
    {
        class IASIIMGReader
        {
        private:
            uint16_t iasi_buffer[64 * 64];
            double last_bbt = 0;
            nlohmann::json calib;

        public:
            int lines;
            std::vector<uint16_t> ir_channel;
            std::vector<double> timestamps_ifov;

        public:
            IASIIMGReader();
            ~IASIIMGReader();
            void work(ccsds::CCSDSPacket &packet);
            void work_calib(ccsds::CCSDSPacket &packet);
            image::Image getIRChannel();
            nlohmann::json getCalib();
        };
    } // namespace iasi
} // namespace metop
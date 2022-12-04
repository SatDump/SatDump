#pragma once

#include "common/ccsds/ccsds.h"
#include <vector>
#include "common/image/image.h"

namespace mats
{
    namespace mats
    {
        class MATSReader
        {
        private:
            int wip_channel = 0;
            std::vector<uint8_t> wip_payload;

            std::vector<uint16_t> nadir_image;
            void process_nadir_imager(image::Image<uint16_t> &img);

        public:
            MATSReader();
            ~MATSReader();

            void work(ccsds::CCSDSPacket &packet, std::string directory);

            int img_cnts[7] = {0, 0, 0, 0, 0, 0, 0};

            int nadir_lines = 0;
            std::vector<double> nadir_timestamps;
            image::Image<uint16_t> getNadirImage();
        };

        static std::string channel_names[7] = {"UV1-short",
                                               "IR3-BG-short",
                                               "IR4-BG-long",
                                               "UV2-short",
                                               "IR1-ABand-center",
                                               "IR2-ABand-total",
                                               "Nadir"};
    } // namespace swap
} // namespace proba
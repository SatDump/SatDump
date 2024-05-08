#pragma once

#include "common/ccsds/ccsds.h"
#include <vector>
#include "common/image/image.h"

namespace proba
{
    namespace vegetation
    {
        class VegetationS
        {
        private:
            std::vector<uint16_t> img_data;

            int frm_size;
            int line_size;

            uint16_t *tmp_words;

        public:
            VegetationS(int frm_size, int line_size);
            ~VegetationS();

            int lines;

            void work(ccsds::CCSDSPacket &packet);
            image::Image getImg();
        };
    } // namespace swap
} // namespace proba
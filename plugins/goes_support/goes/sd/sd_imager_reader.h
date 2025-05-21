#pragma once

#include <cstdint>
#include <vector>
#include "common/image/image.h"
#include <string>

#define FULL_BUF_SZ 10

namespace goes
{
    namespace sd
    {
        class SDImagerReader
        {
        private:
            std::vector<uint16_t> image_vis;
            std::vector<uint16_t> image_ir1;
            std::vector<uint16_t> image_ir2;
            std::vector<uint16_t> image_ir3;
            std::vector<uint16_t> image_ir4;

            int lines = 0;

            std::vector<uint16_t> wip_scanline;

            uint16_t last_status[FULL_BUF_SZ];
            int images_lines = 0;

            bool should_save = false;

            int nsaved = 0;

        public:
            SDImagerReader();
            void work(uint16_t *words);
            image::Image getChannel(int c);
            void try_save(std::string directory, bool force = false);
        };
    }
}
#pragma once

#include <cstdint>
#include <vector>
#include "image/image.h"
#include <string>

namespace dscovr
{
    namespace epic
    {
        class EPICReader
        {
        private:
            std::vector<uint8_t> wip_payload;

        public:
            EPICReader();
            ~EPICReader();

            std::string directory;
            int img_c = 0;

            void work(uint8_t *pkt);
        };
    }
}
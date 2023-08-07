#pragma once

#include <cstdint>
#include <vector>
#include "common/image/image.h"

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
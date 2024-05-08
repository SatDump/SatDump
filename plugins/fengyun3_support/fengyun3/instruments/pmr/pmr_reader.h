#pragma once

#include <cstdint>
#include "common/image/image.h"
#include <vector>
#include <string>

namespace fengyun3
{
    namespace pmr
    {
        class PMRReader
        {
        private:
            const std::string directory;
            image::Image image;

        public:
            PMRReader(std::string directory);
            ~PMRReader();
            int images_count = 0;
            void work(std::vector<uint8_t> &packet);
            void writeCurrent();

            int offset = 0;
        };
    } // namespace virr
} // namespace fengyun
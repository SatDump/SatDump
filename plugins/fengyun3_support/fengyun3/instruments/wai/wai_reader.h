#pragma once

#include <cstdint>
#include "image/image.h"
#include <string>
#include <vector>

namespace fengyun3
{
    namespace wai
    {
        class WAIReader
        {
        private:
            image::Image image;
            const std::string directory;
            void writeCurrent();
            int lines;

        public:
            WAIReader(std::string directory);
            ~WAIReader();
            int images_count = 0;
            void work(std::vector<uint8_t> &packet);
        };
    } // namespace virr
} // namespace fengyun
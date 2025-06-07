#pragma once

#include <cstdint>
#include "image/image.h"
#include <vector>
#include <string>

namespace fengyun3
{
    namespace xeuvi
    {
        class XEUVIReader
        {
        private:
            const std::string directory;
            image::Image image;

        public:
            XEUVIReader(std::string directory);
            ~XEUVIReader();
            int images_count = 0;
            void work(std::vector<uint8_t> &packet);
            void writeCurrent();
        };
    } // namespace virr
} // namespace fengyun
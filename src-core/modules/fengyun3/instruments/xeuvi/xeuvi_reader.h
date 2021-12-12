#pragma once

#include <cstdint>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
#include <vector>
#include <string>

namespace fengyun3
{
    namespace xeuvi
    {
        class XEUVIReader
        {
        private:
            int lines;
            const std::string directory;
            cimg_library::CImg<unsigned short> image;

        public:
            XEUVIReader(std::string directory);
            ~XEUVIReader();
            int images_count = 0;
            void work(std::vector<uint8_t> &packet);
            void writeCurrent();
        };
    } // namespace virr
} // namespace fengyun
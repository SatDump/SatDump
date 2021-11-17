#pragma once

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
#include "suvi_products.h"
#include "../grb_headers.h"
#include <string>

namespace goes
{
    namespace grb
    {
        class GRBSUVIImageAssembler
        {
        private:
            bool hasImage;
            const std::string suvi_directory;
            const products::SUVI::GRBProductSUVI suvi_product;
            double currentTimeStamp;
            cimg_library::CImg<unsigned short> full_image;

            void save();
            void reset();

        public:
            GRBSUVIImageAssembler(std::string abi_dir, products::SUVI::GRBProductSUVI config);
            ~GRBSUVIImageAssembler();
            void pushBlock(GRBImagePayloadHeader header, cimg_library::CImg<unsigned short> &block);
        };
    }
}
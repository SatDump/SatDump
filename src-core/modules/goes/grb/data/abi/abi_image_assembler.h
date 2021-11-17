#pragma once

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
#include "abi_products.h"
#include "../grb_headers.h"
#include <string>

namespace goes
{
    namespace grb
    {
        class GRBABIImageAssembler
        {
        private:
            bool hasImage;
            const std::string abi_directory;
            const products::ABI::GRBProductABI abi_product;
            double currentTimeStamp;
            cimg_library::CImg<unsigned short> full_image;

            void save();
            void reset();

        public:
            GRBABIImageAssembler(std::string abi_dir, products::ABI::GRBProductABI config);
            ~GRBABIImageAssembler();
            void pushBlock(GRBImagePayloadHeader header, cimg_library::CImg<unsigned short> &block);
        };
    }
}
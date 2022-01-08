#pragma once

#include "common/image/image.h"
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
            image::Image<uint16_t> full_image;

            void save();
            void reset();

        public:
            GRBABIImageAssembler(std::string abi_dir, products::ABI::GRBProductABI config);
            ~GRBABIImageAssembler();
            void pushBlock(GRBImagePayloadHeader header, image::Image<uint16_t> &block);
        };
    }
}
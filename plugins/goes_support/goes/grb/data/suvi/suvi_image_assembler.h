#pragma once

#include "common/image/image.h"
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
            image::Image<uint16_t> full_image;

            void save();
            void reset();

        public:
            GRBSUVIImageAssembler(std::string abi_dir, products::SUVI::GRBProductSUVI config);
            ~GRBSUVIImageAssembler();
            void pushBlock(GRBImagePayloadHeader header, image::Image<uint16_t> &block);
        };
    }
}
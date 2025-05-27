#pragma once

#include "image/image.h"
#include "abi_products.h"
#include "../grb_headers.h"
#include <string>
#include "abi_image_composer.h"
#include <memory>
#include "image/image_saving_thread.h"

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
            image::Image full_image;

            void save();
            void reset();

        public:
            GRBABIImageAssembler(std::string abi_dir, products::ABI::GRBProductABI config);
            ~GRBABIImageAssembler();
            void pushBlock(GRBImagePayloadHeader header, image::Image &block);

            std::shared_ptr<ABIComposer> image_composer;

            image::ImageSavingThread *saving_thread;
        };
    }
}
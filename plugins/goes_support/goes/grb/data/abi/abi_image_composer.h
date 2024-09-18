#pragma once

#include "abi_products.h"
#include "common/image/image.h"
#include "common/image/image_saving_thread.h"

namespace goes
{
    namespace grb
    {
        class ABIComposer
        {
        private:
            const std::string abi_directory;
            const products::ABI::ABIScanType abi_product_type;

            double current_timestamp;
            image::Image channel_images[16]; // ABI has 16 channels
            bool has_channels[16];

            void reset();
            bool has_data();
            void save();
            void saveABICompo(image::Image img, std::string name);

        public:
            ABIComposer(std::string dir, products::ABI::ABIScanType abi_type);
            ~ABIComposer();
            void feed_channel(double timestamp, int ch, image::Image &img);

            image::ImageSavingThread *saving_thread;
        };
    }
}
#pragma once

#include "common/lrit/lrit_file.h"
#include "common/lrit/lrit_productizer.h"
#include "image/image.h"
#include "products/image_product.h"
#include "xrit/segment_decoder.h"

namespace satdump
{
    namespace xrit
    {
        class XRITChannelProcessor
        {
        private:
            std::map<std::string, std::shared_ptr<SegmentedImageDecoder>> segmented_decoders;

        private:
            void saveImg(xrit::XRITFileInfo &finfo, image::Image &img);

        public:
            std::string directory = "";
            bool in_memory_mode = false;
            std::map<std::string, std::shared_ptr<products::ImageProduct>> in_memory_products;

        public:
            void push(xrit::XRITFileInfo &finfo, ::lrit::LRITFile &file);
            void flush();
        };
    } // namespace xrit
} // namespace satdump
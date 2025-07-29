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
            struct XRITProd
            {
                std::string directory_path;
                std::string pro_f_path;
                std::shared_ptr<products::ImageProduct> pro;
            };

        private:
            XRITProd setupProduct(xrit::XRITFileInfo &meta);
            bool saveMeta(xrit::XRITFileInfo &meta, ::lrit::LRITFile &file);
            void saveImg(xrit::XRITFileInfo &meta, image::Image &img);

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
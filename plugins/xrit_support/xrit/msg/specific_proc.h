#pragma once

#include "products/image_product.h"

namespace satdump
{
    namespace xrit
    {
        void tryApplyMSGCalib(std::shared_ptr<products::ImageProduct> &pro);
        void tryApplyMSG_HRVProcessing(std::shared_ptr<products::ImageProduct> &pro, image::Image &img, std::string *nameptr = nullptr);
    } // namespace xrit
} // namespace satdump
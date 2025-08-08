#pragma once

/**
 * @file specific_proc.h
 * @brief MSG-Specific processing function
 */

#include "products/image_product.h"

namespace satdump
{
    namespace xrit
    {
        /**
         * @brief Attempt to populate calibration information for each
         * channel, if possible (the PRO file has been received).
         * @param pro product to work on
         */
        void tryApplyMSGCalib(std::shared_ptr<products::ImageProduct> &pro);

        /**
         * @brief HRV only scans parts of the globe unlike other channels,
         * and needs to be processed using the EPI file back into a proper full disk
         * or RSS scan. This function does this.
         * Note : if HRV was received, but no header, the channel is named 12_ to avoid
         * composites and other processing using it - in a state where it's not usable.
         * @param pro product to work on
         * @param img the HRV channel itself
         * @param nameptr the xRIT File Info nameptr to change it back to "12" if processing
         * succeeded
         */
        void tryApplyMSG_HRVProcessing(std::shared_ptr<products::ImageProduct> &pro, image::Image &img, std::string *nameptr = nullptr);
    } // namespace xrit
} // namespace satdump
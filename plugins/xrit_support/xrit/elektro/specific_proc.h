#pragma once

/**
 * @file specific_proc.h
 * @brief ELEKTRO-Specific processing function
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
        void tryApplyELEKTROCalib(std::shared_ptr<products::ImageProduct> &pro);
    } // namespace xrit
} // namespace satdump
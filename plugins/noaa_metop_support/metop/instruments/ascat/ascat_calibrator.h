#pragma once

#include "common/calibration.h"
#include "metop/instruments/ascat/ascat_reader.h"
#include "nlohmann/json.hpp"
#include "products/image/image_calibrator.h"

namespace metop
{
    namespace ascat
    {
        class MetOpASCATCalibrator : public satdump::products::ImageCalibrator
        {
        private:
           // nlohmann::json d_vars;
      

        public:
            MetOpASCATCalibrator(satdump::products::ImageProduct *p, nlohmann::json c) : satdump::products::ImageCalibrator(p, c)
            {
              //  d_vars = d_cfg["vars"];
            }

            void init() {}

            double compute(int /* channel */, int /* pos_x */, int pos_y, uint32_t px_val)
            {
                return parse_uint_to_float(px_val);
            }
        };
    } // namespace iasi
} // namespace metop

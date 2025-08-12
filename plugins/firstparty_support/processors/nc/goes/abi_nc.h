#pragma once

#include "../../processor.h"
#include "products/image_product.h"

namespace satdump
{
    namespace firstparty
    {
        class ABINcProcessor : public FirstPartyProductProcessor
        {
        private:
            struct ParsedGOESRABI
            {
                image::Image img;
                int channel;
                double longitude;
                double x_add_offset, y_add_offset;
                double x_scale_factor, y_scale_factor;
                double perspective_point_height;
                double calibration_scale;
                double calibration_offset;
                double kappa;
                time_t timestamp;
                std::string goes_sat;
                std::string time_coverage_start;
            };

            float center_longitude = 0;
            std::vector<ParsedGOESRABI> all_images;

            time_t prod_timestamp = 0;
            std::string satellite = "Unknown GOES-R";

            ParsedGOESRABI parse_goes_abi_netcdf(std::vector<uint8_t> &data);

        public:
            void ingestFile(std::vector<uint8_t> file);
            std::shared_ptr<satdump::products::Product> getProduct();
        };
    } // namespace firstparty
} // namespace satdump
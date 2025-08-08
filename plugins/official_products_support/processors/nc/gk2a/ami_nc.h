#pragma once

#include "../../processor.h"
#include "products/image_product.h"

namespace satdump
{
    namespace official
    {
        class AMINcProcessor : public OfficialProductProcessor
        {
        private:
            struct ParsedGK2AAMI
            {
                image::Image img;
                std::string channel;
                double longitude;
                double cfac, lfac;
                double coff, loff;
                double nominal_satellite_height;
                double calibration_scale;
                double calibration_offset;
                double kappa;
                time_t timestamp;
                std::string sat_name;
            };

            std::vector<ParsedGK2AAMI> all_images;

            ParsedGK2AAMI parse_gk2a_ami_netcdf(std::vector<uint8_t> &data);

        public:
            void ingestFile(std::vector<uint8_t> file);
            std::shared_ptr<satdump::products::Product> getProduct();
        };
    } // namespace official
} // namespace satdump
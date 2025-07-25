#pragma once

#include "../../processor.h"
#include "products/image_product.h"

namespace satdump
{
    namespace official
    {
        class FCINcProcessor : public OfficialProductProcessor
        {
        private:
            struct ParsedMTGFCI
            {
                image::Image imgs[16];
                int start_row[16];
                double longitude;
                double calibration_scale[16];
                double calibration_offset[16];
                std::string time_coverage_start;
                std::string platform_name;
            };

            double calibration_scale[16] = {0};
            double calibration_offset[16] = {0};

            float center_longitude = 0;
            std::vector<ParsedMTGFCI> all_images;

            time_t prod_timestamp = 0;
            std::string satellite = "Unknown MTG-I";

            ParsedMTGFCI parse_mtg_fci_netcdf_fulldisk(std::vector<uint8_t> data);

        public:
            void ingestFile(std::vector<uint8_t> file);
            std::shared_ptr<satdump::products::Product> getProduct();
        };
    } // namespace official
} // namespace satdump
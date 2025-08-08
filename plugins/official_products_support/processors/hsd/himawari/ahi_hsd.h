#pragma once

#include "../../processor.h"
#include "products/image_product.h"

namespace satdump
{
    namespace official
    {
        class AHIHsdProcessor : public OfficialProductProcessor
        {
        private:
            struct ParsedHimawariAHI
            {
                bool header_parsed = false;
                image::Image img;
                uint16_t channel;
                double longitude;
                int cfac, lfac;
                float coff, loff;
                double altitude;
                double wavenumber;
                double calibration_scale;
                double calibration_offset;
                double kappa;
                time_t timestamp;
                std::string sat_name;
            };

            enum HSDBlocks
            {
                HSD_BASIC_INFO,
                HSD_DATA_INFO,
                HSD_PROJ_INFO,
                HSD_NAV_INFO,
                HSD_CAL_INFO,
                HSD_INTER_CAL_INFO,
                HSD_SEGMENT_INFO,
                HSD_NAV_CORRECTION_INFO,
                HSD_OBS_TIME_INFO,
                HSD_ERROR_INFO,
                HSD_SPARE_BLOCK,
                HSD_DATA_BLOCK
            };

            ParsedHimawariAHI all_images[16];

            void parse_himawari_ahi_hsd(std::vector<uint8_t> &data);

        public:
            void ingestFile(std::vector<uint8_t> file);
            std::shared_ptr<satdump::products::Product> getProduct();
        };
    } // namespace official
} // namespace satdump
#include "metimage_nc.h"
#include "../../hdf_utils.h"
#include "common/utils.h"
#include "image/image.h"
#include "logger.h"
#include <H5Cpp.h>
#include <H5LTpublic.h>
#include <cstddef>
#include <cstdint>
#include <string>

namespace satdump
{
    namespace firstparty
    {
        image::Image get_img_from_hdf(H5::H5File &file, std::string path);

        void METIMAGENcProcessor::ingestFile(std::vector<uint8_t> vec)
        {
            H5::H5File file(H5LTopen_file_image(vec.data(), vec.size(), H5F_ACC_RDONLY));

            double timestamp = -1;
            {
                std::string date_str;
                auto att = file.openAttribute("sensing_start_time_utc");
                att.read(att.getStrType(), date_str);

                std::tm timeS;
                memset(&timeS, 0, sizeof(std::tm));
                int ign;
                if (sscanf(date_str.c_str(), "%4d-%2d-%2d %2d:%2d:%2d.%3d", &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec, &ign) == 7)
                {
                    timeS.tm_year -= 1900;
                    timeS.tm_mon -= 1;
                    timeS.tm_isdst = -1;
                    timestamp = timegm(&timeS);
                }
            }

            std::string source = "UKN";

            {
                std::string satid;
                auto att = file.openAttribute("spacecraft");
                att.read(att.getStrType(), satid);

                if (satid == "SGA1")
                    source = "A1";
                else if (satid == "SGA2")
                    source = "A2";
                else if (satid == "SGA3")
                    source = "A3";
            }

            // Init as needed
            metimage_products = std::make_shared<satdump::products::ImageProduct>();
            metimage_products->instrument_name = "metimage";
            metimage_products->set_product_timestamp(timestamp);
            metimage_products->set_product_source("MetOp-SG " + source);

            // There, we want only Data
            if (file.nameExists("data/measurement_data"))
            {
                std::vector<std::pair<int, std::string>> opts = {

                    {0, "vii_443"},    //
                    {1, "vii_555"},    //
                    {2, "vii_668"},    //
                    {3, "vii_752"},    //
                    {4, "vii_763"},    //
                    {5, "vii_865"},    //
                    {6, "vii_914"},    //
                    {7, "vii_1240"},   //
                    {8, "vii_1375"},   //
                    {9, "vii_1630"},   //
                    {10, "vii_2250"},  //
                    {11, "vii_3740"},  //
                    {12, "vii_3959"},  //
                    {13, "vii_4050"},  //
                    {14, "vii_6725"},  //
                    {15, "vii_7325"},  //
                    {16, "vii_8540"},  //
                    {17, "vii_10690"}, //
                    {18, "vii_12020"}, //
                    {19, "vii_13345"}, //
                };

                for (auto &ch : opts)
                {
                    if (file.nameExists("data/measurement_data/" + ch.second))
                    {
                        auto img = get_img_from_hdf(file, "data/measurement_data/" + ch.second);

                        int max_val = hdfpp_read_attribute_int(file.openDataSet("data/measurement_data/" + ch.second).openAttribute("valid_max"));

                        int shift = 0;
                        if (max_val == 1023)
                            shift = 6;
                        else if (max_val == 4095)
                            shift = 4;
                        else if (max_val == 8191)
                            shift = 3;
                        else if (max_val == 16383)
                            shift = 2;

                        for (size_t i = 0; i < img.size(); i++)
                            img.set(i, img.get(i) << shift);

                        int i = ch.first;
                        metimage_products->images.push_back({i, "METImage-" + std::to_string(i + 1), std::to_string(i + 1), img, 16, satdump::ChannelTransform().init_none()});
                    }
                }
            }
        }

        std::shared_ptr<satdump::products::Product> METIMAGENcProcessor::getProduct() { return metimage_products; }
    } // namespace firstparty
} // namespace satdump
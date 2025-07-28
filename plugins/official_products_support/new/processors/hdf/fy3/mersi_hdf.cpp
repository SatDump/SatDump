#include "mersi_hdf.h"
#include "image/image.h"
#include "logger.h"
#include "utils/string.h"
#include <H5Cpp.h>
#include <H5LTpublic.h>
#include <cstdint>
#include "common/utils.h"
#include <string>

namespace satdump
{
    namespace official
    {
        // TODOREWORK probably make this generic?
        std::vector<image::Image> get_multi_img_from_hdf(H5::H5File &file, std::string path)
        {
            H5::DataSet s = file.openDataSet(path);

            if (s.getSpace().getSimpleExtentNdims() != 3)
            {
                logger->error("Rank is not 3!");
                return {};
            }

            hsize_t image_dims[3];
            s.getSpace().getSimpleExtentDims(image_dims);

            std::vector<uint16_t> tmp_all_channels(image_dims[0] * image_dims[1] * image_dims[2], 0);
            s.read(tmp_all_channels.data(), H5::PredType::NATIVE_UINT16);

            std::vector<image::Image> imgs;
            for (int i = 0; i < image_dims[0]; i++)
                imgs.push_back(image::Image(tmp_all_channels.data() + image_dims[2] * image_dims[1] * i, 16, image_dims[2], image_dims[1], 1));

            return imgs;
        }

        image::Image get_img_from_hdf(H5::H5File &file, std::string path);

        void MERSIHdfProcessor::ingestFile(std::vector<uint8_t> vec)
        {
            H5::H5File file(H5LTopen_file_image(vec.data(), vec.size(), H5F_ACC_RDONLY));

            double timestamp = -1;
            {
                std::string date_str;
                auto att = file.openAttribute("Observing Beginning Date");
                att.read(att.getStrType(), date_str);

                std::string time_str;
                auto att2 = file.openAttribute("Observing Beginning Time");
                att2.read(att2.getStrType(), time_str);

                std::tm timeS;
                memset(&timeS, 0, sizeof(std::tm));
                if (sscanf(date_str.c_str(), "%4d-%2d-%2d", &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday) != 3)
                    logger->error("Parse Date Error!");
                if (sscanf(time_str.c_str(), "%2d:%2d:%2d", &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec) != 3)
                    logger->error("Parse Time Error!");
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                timestamp = timegm(&timeS);
            }

            std::string name_str;
            auto att = file.openAttribute("Satellite Name");
            att.read(att.getStrType(), name_str);

            if (name_str == "FY-3D")
                mersi_type = MERSI_2;
            else if (name_str == "FY-3E")
                mersi_type = MERSI_LL;
            else if (name_str == "FY-3F")
                mersi_type = MERSI_3;
            else if (name_str == "FY-3G")
                mersi_type = MERSI_RM;

            // Init as needed
            if (!mersi_products)
            {
                mersi_products = std::make_shared<satdump::products::ImageProduct>();
                if (mersi_type == MERSI_RM)
                    mersi_products->instrument_name = "mersirm";
                else if (mersi_type == MERSI_3)
                    mersi_products->instrument_name = "mersi3";
                mersi_products->set_product_timestamp(timestamp);
                mersi_products->set_product_source(name_str);
            }

            // From now on, we want only Data
            if (!file.nameExists("Data"))
                return;

            // Process depending on MERSI type
            if (mersi_type == MERSI_RM)
            {
                if (file.nameExists("Data/EV_Reflectance"))
                {
                    auto ch1_5 = get_multi_img_from_hdf(file, "Data/EV_Reflectance");
                    for (int i = 0; i < ch1_5.size(); i++)
                        mersi_products->images.push_back({i, "MERSIRM-" + std::to_string(i + 1), std::to_string(i + 1), ch1_5[i], 16, satdump::ChannelTransform().init_none()});
                }
                if (file.nameExists("Data/EV_Emissive"))
                {
                    auto ch6_8 = get_multi_img_from_hdf(file, "Data/EV_Emissive");
                    for (int i = 5; i < 5 + ch6_8.size(); i++)
                        mersi_products->images.push_back({i, "MERSIRM-" + std::to_string(i + 1), std::to_string(i + 1), ch6_8[i - 5], 16, satdump::ChannelTransform().init_none()});
                }
            }
            else if (mersi_type == MERSI_3)
            {
                if (file.nameExists("Data/EV_250_RefSB_b1"))
                {
                    auto img = get_img_from_hdf(file, "Data/EV_250_RefSB_b1");
                    for (size_t i = 0; i < img.size(); i++)
                        img.set(i, img.get(i) << 4);
                    mersi_products->images.push_back({0, "MERSI3-1", "1", img, 12, satdump::ChannelTransform().init_none()});
                }
                if (file.nameExists("Data/EV_250_RefSB_b2"))
                {
                    auto img = get_img_from_hdf(file, "Data/EV_250_RefSB_b2");
                    for (size_t i = 0; i < img.size(); i++)
                        img.set(i, img.get(i) << 4);
                    mersi_products->images.push_back({1, "MERSI3-2", "2", img, 12, satdump::ChannelTransform().init_none()});
                }
                if (file.nameExists("Data/EV_250_RefSB_b3"))
                {
                    auto img = get_img_from_hdf(file, "Data/EV_250_RefSB_b3");
                    for (size_t i = 0; i < img.size(); i++)
                        img.set(i, img.get(i) << 4);
                    mersi_products->images.push_back({2, "MERSI3-3", "3", img, 12, satdump::ChannelTransform().init_none()});
                }
                if (file.nameExists("Data/EV_250_RefSB_b4"))
                {
                    auto img = get_img_from_hdf(file, "Data/EV_250_RefSB_b4");
                    for (size_t i = 0; i < img.size(); i++)
                        img.set(i, img.get(i) << 4);
                    mersi_products->images.push_back({3, "MERSI3-4", "4", img, 12, satdump::ChannelTransform().init_none()});
                }
                if (file.nameExists("Data/EV_250_Emissive_b24"))
                {
                    auto img = get_img_from_hdf(file, "Data/EV_250_Emissive_b24");
                    mersi_products->images.push_back({4, "MERSI3-5", "5", img, 16, satdump::ChannelTransform().init_none()});
                }
                if (file.nameExists("Data/EV_250_Emissive_b25"))
                {
                    auto img = get_img_from_hdf(file, "Data/EV_250_Emissive_b25");
                    mersi_products->images.push_back({5, "MERSI3-6", "6", img, 16, satdump::ChannelTransform().init_none()});
                }
                if (file.nameExists("Data/EV_1KM_RefSB"))
                {
                    auto ch5_19 = get_multi_img_from_hdf(file, "Data/EV_1KM_RefSB");
                    for (int i = 6; i < 6 + ch5_19.size(); i++)
                    {
                        for (size_t i2 = 0; i2 < ch5_19[i - 6].size(); i2++)
                            ch5_19[i - 6].set(i2, ch5_19[i - 6].get(i2) << 4);
                        mersi_products->images.push_back({i, "MERSI3-" + std::to_string(i + 1), std::to_string(i + 1), ch5_19[i - 6], 12, satdump::ChannelTransform().init_affine(4, 4, 0, 0)});
                    }
                }
                if (file.nameExists("Data/EV_1KM_Emissive"))
                {
                    auto ch20_23 = get_multi_img_from_hdf(file, "Data/EV_1KM_Emissive");
                    for (int i = 21; i < 21 + ch20_23.size(); i++)
                        mersi_products->images.push_back({i, "MERSI3-" + std::to_string(i + 1), std::to_string(i + 1), ch20_23[i - 21], 16, satdump::ChannelTransform().init_affine(4, 4, 0, 0)});
                }
            }
        }

        std::shared_ptr<satdump::products::Product> MERSIHdfProcessor::getProduct() { return mersi_products; }
    } // namespace official
} // namespace satdump
#include "ghi_hdf.h"
#include "image/image.h"
#include "logger.h"
#include <H5Cpp.h>
#include <H5LTpublic.h>
#include <cstdint>
#include <exception>
#include <string>

#if defined(_WIN32)
#include <time.h>

inline time_t timegm(struct tm *const t) { return _mkgmtime(t); }
#endif

namespace satdump
{
    namespace firstparty
    {
        // TODOREWORK probably make this generic?
        std::vector<image::Image> get_multi_img_from_hdf(H5::H5File &file, std::string path);

        image::Image get_img_from_hdf(H5::H5File &file, std::string path);

        void GHIHdfProcessor::ingestFile(std::vector<uint8_t> vec)
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

            // Init as needed
            if (!ghi_products)
            {
                ghi_products = std::make_shared<satdump::products::ImageProduct>();
                ghi_products->instrument_name = "ghi";
                ghi_products->set_product_timestamp(timestamp);
                ghi_products->set_product_source(name_str);
            }

            // There, we want only Data
            if (file.nameExists("Data"))
            {
                for (int i = 0; i < 7; i++)
                {
                    if (file.nameExists("Data/NOMChannel0" + std::to_string(i + 1)))
                    {
                        auto img = get_img_from_hdf(file, "Data/NOMChannel0" + std::to_string(i + 1));
                        for (size_t i = 0; i < img.size(); i++)
                            img.set(i, img.get(i) << 4);

                        try
                        {
                            auto &h = ghi_products->get_channel_image(i);
                            if (h.image.width() < img.width())
                                h.image = img;
                        }
                        catch (std::exception &e)
                        {
                            ghi_products->images.push_back({i, "GHI-" + std::to_string(i + 1), std::to_string(i + 1), img, 16, satdump::ChannelTransform().init_none()});
                        }
                    }
                }
            }
        }

        std::shared_ptr<satdump::products::Product> GHIHdfProcessor::getProduct() { return ghi_products; }
    } // namespace firstparty
} // namespace satdump
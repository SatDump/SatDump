#include "svissr_hdf.h"
#include "image/image.h"
#include "logger.h"
#include "utils/string.h"
#include <H5Cpp.h>
#include <H5LTpublic.h>
#include <ctime>

namespace satdump
{
    namespace official
    {
        // TODOREWORK probably make this generic?
        image::Image get_img_from_hdf(H5::H5File &file, std::string path)
        {
            H5::DataSet s = file.openDataSet(path);

            if (s.getSpace().getSimpleExtentNdims() != 2)
            {
                logger->error("Rank is not 2!");
                return image::Image();
            }

            hsize_t image_dims[2];
            s.getSpace().getSimpleExtentDims(image_dims);
            image::Image img(16, image_dims[1], image_dims[0], 1);
            s.read(img.raw_data(), H5::PredType::NATIVE_UINT16);

            return img;
        }

        void SVISSRHdfProcessor::ingestFile(std::vector<uint8_t> vec)
        {
            H5::H5File file(H5LTopen_file_image(vec.data(), vec.size(), H5F_ACC_RDONLY));

            struct NomFileInfo
            {
                unsigned short StartYear;
                unsigned short StartMonth;
                unsigned short StartDay;
                unsigned short StartHour;
                unsigned short StartMinute;
                unsigned short StartSecond;
                char Satellite[13];
            };

            NomFileInfo hdr;

            { // Read Header
                H5::CompType type(sizeof(NomFileInfo));
                type.insertMember("StartYear", HOFFSET(NomFileInfo, StartYear), H5::PredType::NATIVE_USHORT);
                type.insertMember("StartMonth", HOFFSET(NomFileInfo, StartMonth), H5::PredType::NATIVE_USHORT);
                type.insertMember("StartDay", HOFFSET(NomFileInfo, StartDay), H5::PredType::NATIVE_USHORT);
                type.insertMember("StartHour", HOFFSET(NomFileInfo, StartHour), H5::PredType::NATIVE_USHORT);
                type.insertMember("StartMinute", HOFFSET(NomFileInfo, StartMinute), H5::PredType::NATIVE_USHORT);
                type.insertMember("StartSecond", HOFFSET(NomFileInfo, StartSecond), H5::PredType::NATIVE_USHORT);
                hid_t strtype = H5Tcopy(H5T_C_S1);
                H5Tset_size(strtype, 13);
                type.insertMember("Satellite", HOFFSET(NomFileInfo, Satellite), strtype);

                file.openDataSet("NomFileInfo").read(&hdr, type);
            }

            std::tm timeS;
            memset(&timeS, 0, sizeof(std::tm));
            timeS.tm_year = hdr.StartYear;
            timeS.tm_mon = hdr.StartMonth;
            timeS.tm_mday = hdr.StartDay;
            timeS.tm_hour = hdr.StartHour;
            timeS.tm_min = hdr.StartMinute;
            timeS.tm_sec = hdr.StartSecond;
            timeS.tm_year -= 1900;
            timeS.tm_mon -= 1;
            double timestamp = timegm(&timeS);

            std::string satellite_name(hdr.Satellite);
            replaceAllStr(satellite_name, "FY2", "FY-2");

            svissr_products = std::make_shared<satdump::products::ImageProduct>();
            svissr_products->instrument_name = "fy2-svissr";
            svissr_products->set_product_timestamp(timestamp);
            svissr_products->set_product_source(satellite_name);

            svissr_products->images.push_back({0, "SVISSR-1", "1", get_img_from_hdf(file, "NOMChannelVIS1KM"), 6, satdump::ChannelTransform().init_none()});
            svissr_products->images.push_back({1, "SVISSR-2", "2", get_img_from_hdf(file, "NOMChannelIR1"), 10, satdump::ChannelTransform().init_affine(4, 4, 0, 0)});
            svissr_products->images.push_back({2, "SVISSR-3", "3", get_img_from_hdf(file, "NOMChannelIR2"), 10, satdump::ChannelTransform().init_affine(4, 4, 0, 0)});
            svissr_products->images.push_back({3, "SVISSR-4", "4", get_img_from_hdf(file, "NOMChannelIR3"), 10, satdump::ChannelTransform().init_affine(4, 4, 0, 0)});
            svissr_products->images.push_back({4, "SVISSR-5", "5", get_img_from_hdf(file, "NOMChannelIR4"), 10, satdump::ChannelTransform().init_affine(4, 4, 0, 0)});

            // Convert so it matches official data (and uses the full range). TODOREWORK?
            for (auto &h : svissr_products->images)
            {
                if (h.abs_index != 0)
                {
                    auto &img = h.image;
                    for (size_t i = 0; i < img.size(); i++)
                        if (img.get(i) == 65535)
                            img.set(i, 0);
                        else
                            img.set(i, 65535 - (img.get(i) << 6));
                }
                else
                {
                    auto &img = h.image;
                    for (size_t i = 0; i < img.size(); i++)
                        if (img.get(i) == 65535)
                            img.set(i, 0);
                        else
                            img.set(i, img.get(i) << 10);
                }
            }
        }

        std::shared_ptr<satdump::products::Product> SVISSRHdfProcessor::getProduct() { return svissr_products; }
    } // namespace official
} // namespace satdump
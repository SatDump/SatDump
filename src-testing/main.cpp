/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "logger.h"
#include "common/image/image.h"
#include "libs/rapidxml.hpp"
#include <filesystem>
#include <fstream>

// XML Parser
rapidxml::xml_document<> doc;
rapidxml::xml_node<> *root_node = NULL;

int main(int argc, char *argv[])
{
    initLogger();

    std::filesystem::recursive_directory_iterator pluginIterator(argv[1]);
    std::error_code iteratorError;
    while (pluginIterator != std::filesystem::recursive_directory_iterator())
    {
        if (pluginIterator->path().filename().string().find(".png") != std::string::npos)
        {
            std::string png_path = pluginIterator->path().string();
            std::string xml_path = png_path.substr(0, png_path.size() - 4) + ".xml";

            std::string png_path2 = png_path.substr(0, png_path.size() - 4) + "_CALIB.png";

            if (!std::filesystem::exists(xml_path))
            {
                pluginIterator.increment(iteratorError);
                continue;
            }

            image::Image<uint16_t> suvi_image;
            suvi_image.load_png(png_path);

            std::ifstream xml_ifs(xml_path);
            std::string xml_content((std::istreambuf_iterator<char>(xml_ifs)),
                                    (std::istreambuf_iterator<char>()));

            doc.parse<0>((char *)xml_content.c_str());
            root_node = doc.first_node("netcdf");

            float img_min, img_max;
            std::string sci_obj;

            for (rapidxml::xml_node<> *sat_node = root_node->first_node("variable"); sat_node; sat_node = sat_node->next_sibling())
            {
                //  for (rapidxml::xml_node<> *tle_node = sat_tle_node->first_node("attribute"); tle_node; tle_node = tle_node->next_sibling())
                if (sat_node->first_attribute("name") != NULL)
                {
                    if (std::string(sat_node->first_attribute("name")->value()) == "IMG_MIN")
                        img_min = std::stof(sat_node->first_node("values")->value());
                    if (std::string(sat_node->first_attribute("name")->value()) == "IMG_MAX")
                        img_max = std::stof(sat_node->first_node("values")->value());
                    if (std::string(sat_node->first_attribute("name")->value()) == "SCI_OBJ")
                        sci_obj = std::string(sat_node->first_node("values")->value());
                }
            }

            if (sci_obj != "Fe_XII_195.1A_long_exposure")
            {
                pluginIterator.increment(iteratorError);
                continue;
            }

            logger->info("Image Max {:f} Min {:f}, product {:s}", img_max, img_min, sci_obj.c_str());

            for (size_t px = 0; px < suvi_image.width() * suvi_image.height(); px++)
            {
                double radiance = img_min + (suvi_image[px] / 65535.0) * (img_max - img_min);

                const double mmm_min = -1;
                const double mmm_max = 24;

                double calib_v = ((radiance - mmm_min) / (mmm_max - mmm_min)) * 65535.0;

                if (calib_v < 0)
                    calib_v = 0;
                if (calib_v > 65535)
                    calib_v = 65535;

                suvi_image[px] = calib_v;
            }

            suvi_image.save_png(png_path2);
        }

        pluginIterator.increment(iteratorError);
    }

#if 0
    image::Image<uint8_t> image_1, image_2, image_3, image_rgb;

    image_1.load_png(argv[1]);
    image_2.load_png(argv[2]);
    image_3.load_png(argv[3]);

    logger->info("Processing");
    image_rgb.init(image_1.width(), image_1.height(), 3);

    image_rgb.draw_image(0, image_3, -30, 12);
    image_rgb.draw_image(1, image_2, 0);
    image_rgb.draw_image(2, image_1, 23, -17);

    logger->info("Saving");
    image_rgb.save_png("msu_compo.png");
#endif
}
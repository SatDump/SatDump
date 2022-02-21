#include "lrit_data_decoder.h"
#include "resources.h"
#include "imgui/imgui_image.h"
#include "logger.h"
#include "common/image/hue_saturation.h"

namespace elektro
{
    namespace lrit
    {
        ELEKTRO321Composer::ELEKTRO321Composer()
        {
            time1 = 0;
            time2 = 0;

            imageStatus = IDLE;
        }

        ELEKTRO321Composer::~ELEKTRO321Composer()
        {
        }

        void ELEKTRO321Composer::generateCompo()
        {
            imageStatus = RECEIVING;

            // Not really necessary, but good to be safe
            if (ch1.height() > 0)
            {
                ch2.resize(ch1.width(), ch1.height());
                ch3.resize(ch1.width(), ch1.height());
            }
            else if (ch2.height() > 0)
            {
                ch1.resize(ch2.width(), ch2.height());
                ch3.resize(ch2.width(), ch2.height());
            }
            else
            {
                ch2.resize(ch3.width(), ch3.height());
                ch1.resize(ch3.width(), ch3.height());
            }

            compo321 = image::Image<uint8_t>(ch1.width(), ch1.height(), 3); // Init image

            compo321.draw_image(0, ch3);
            compo321.draw_image(1, ch2);
            compo321.draw_image(2, ch1);

            compo231 = image::Image<uint8_t>(ch1.width(), ch1.height(), 3); // Init image

            compo231.draw_image(0, ch2);
            compo231.draw_image(1, ch3);
            compo231.draw_image(2, ch1);

            compoNC = compo321;

            hasData = true;

            // If the UI is active, update texture
            if (textureID > 0)
            {
                // Downscale image
                img_height = 1000;
                img_width = 1000;
                image::Image<uint8_t> imageScaled = compo321;
                imageScaled.resize(img_width, img_height);
                uchar_to_rgba(imageScaled.data(), textureBuffer, img_height * img_width, 3);
                hasToUpdate = true;
            }
        }

        void ELEKTRO321Composer::push3(image::Image<uint8_t> img, time_t time)
        {
            ch3 = img;
            time3 = time;

            if (time1 == time2 && time2 == time3)
                generateCompo();
        }

        void ELEKTRO321Composer::push2(image::Image<uint8_t> img, time_t time)
        {
            ch2 = img;
            time2 = time;

            if (time1 == time2 && time2 == time3)
                generateCompo();
        }

        void ELEKTRO321Composer::push1(image::Image<uint8_t> img, time_t time)
        {
            ch1 = img;
            time1 = time;

            if (time1 == time2 && time2 == time3)
                generateCompo();
        }

        void ELEKTRO321Composer::save(std::string directory)
        {
            imageStatus = SAVING;
            logger->info("Writing image " + directory + "/IMAGES/" + filename321 + ".png" + "...");
            compo321.save_png(std::string(directory + "/IMAGES/" + filename321 + ".png").c_str());

            logger->info("Writing image " + directory + "/IMAGES/" + filename231 + ".png" + "...");
            compo231.save_png(std::string(directory + "/IMAGES/" + filename231 + ".png").c_str());

            logger->info("Writing image " + directory + "/IMAGES/" + filenameNC + ".png" + "...");
            image::HueSaturation hueTuning;
            hueTuning.hue[image::HUE_RANGE_YELLOW] = -45.0 / 180.0;
            hueTuning.hue[image::HUE_RANGE_RED] = 90.0 / 180.0;
            hueTuning.overlap = 100.0 / 100.0;
            image::hue_saturation(compoNC, hueTuning);
            logger->warn("This natural color composite was originally found by Derek (@dereksgc)!");
            compoNC.save_png(std::string(directory + "/IMAGES/" + filenameNC + ".png").c_str());
            hasData = false;
            imageStatus = IDLE;
        }
    }
}
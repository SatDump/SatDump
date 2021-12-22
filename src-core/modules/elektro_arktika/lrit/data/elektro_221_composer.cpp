#include "lrit_data_decoder.h"
#include "resources.h"
#include "imgui/imgui_image.h"
#include "logger.h"

namespace elektro
{
    namespace lrit
    {
        ELEKTRO221Composer::ELEKTRO221Composer()
        {
            time1 = 0;
            time2 = 0;

            imageStatus = IDLE;
        }

        ELEKTRO221Composer::~ELEKTRO221Composer()
        {
        }

        void ELEKTRO221Composer::generateCompo()
        {
            imageStatus = RECEIVING;

            // Not really necessary, but good to be safe
            if (ch1.height() > 0)
                ch2.resize(ch1.width(), ch1.height());
            else
                ch1.resize(ch2.width(), ch2.height());

            compo221 = image::Image<uint8_t>(ch1.width(), ch1.height(), 3); // Init image

            compo221.draw_image(0, ch2);
            compo221.draw_image(1, ch2);
            compo221.draw_image(2, ch1);

            hasData = true;

            // If the UI is active, update texture
            if (textureID > 0)
            {
                // Downscale image
                img_height = 1000;
                img_width = 1000;
                image::Image<uint8_t> imageScaled = compo221;
                imageScaled.resize(img_width, img_height);
                uchar_to_rgba(imageScaled.data(), textureBuffer, img_height * img_width, 3);
                hasToUpdate = true;
            }
        }

        void ELEKTRO221Composer::push2(image::Image<uint8_t> img, time_t time)
        {
            ch2 = img;
            time2 = time;

            if (time1 == time2)
                generateCompo();
        }

        void ELEKTRO221Composer::push1(image::Image<uint8_t> img, time_t time)
        {
            ch1 = img;
            time1 = time;

            if (time1 == time2)
                generateCompo();
        }

        void ELEKTRO221Composer::save(std::string directory)
        {
            imageStatus = SAVING;
            logger->info("Writing image " + directory + "/IMAGES/" + filename + ".png" + "...");
            compo221.save_png(std::string(directory + "/IMAGES/" + filename + ".png").c_str());
            hasData = false;
            imageStatus = IDLE;
        }
    }
}
#include "lrit_data_decoder.h"
#include "resources.h"
#include "imgui/imgui_image.h"
#include "logger.h"

namespace elektro
{
    namespace lrit
    {
        ELEKTRO321Composer::ELEKTRO321Composer()
        {
            time6 = 0;
            time9 = 0;

            imageStatus = IDLE;
        }

        ELEKTRO321Composer::~ELEKTRO321Composer()
        {
        }

        void ELEKTRO321Composer::generateCompo()
        {
            imageStatus = RECEIVING;

            // Not really necessary, but good to be safe
            if (ch9.height() > 0)
            {
                ch6.resize(ch9.width(), ch9.height());
                ch7.resize(ch9.width(), ch9.height());
            }
            else if (ch7.height() > 0)
            {
                ch9.resize(ch7.width(), ch7.height());
                ch6.resize(ch7.width(), ch7.height());
            }
            else
            {
                ch9.resize(ch6.width(), ch6.height());
                ch7.resize(ch6.width(), ch6.height());
            }

            compo321 = cimg_library::CImg<unsigned char>(ch6.width(), ch6.height(), 1, 3); // Init image

            compo321.draw_image(0, 0, 0, 0, ch7);
            compo321.draw_image(0, 0, 0, 1, ch9);
            compo321.draw_image(0, 0, 0, 2, ch6);

            hasData = true;

            // If the UI is active, update texture
            if (textureID > 0)
            {
                // Downscale image
                img_height = 1000;
                img_width = 1000;
                cimg_library::CImg<unsigned char> imageScaled = compo321;
                imageScaled.resize(img_width, img_height);
                uchar_to_rgba(imageScaled, textureBuffer, img_height * img_width, 3);
                hasToUpdate = true;
            }
        }

        void ELEKTRO321Composer::push6(cimg_library::CImg<unsigned char> img, time_t time)
        {
            ch6 = img;
            time6 = time;

            if (time6 == time9 && time7 == time9)
                generateCompo();
        }

        void ELEKTRO321Composer::push7(cimg_library::CImg<unsigned char> img, time_t time)
        {
            ch7 = img;
            time7 = time;

            if (time6 == time9 && time7 == time9)
                generateCompo();
        }

        void ELEKTRO321Composer::push9(cimg_library::CImg<unsigned char> img, time_t time)
        {
            ch9 = img;
            time9 = time;

            if (time6 == time9 && time7 == time9)
                generateCompo();
        }

        void ELEKTRO321Composer::save(std::string directory)
        {
            imageStatus = SAVING;
            logger->info("Writing image " + directory + "/IMAGES/" + filename + ".png" + "...");
            compo321.save_png(std::string(directory + "/IMAGES/" + filename + ".png").c_str());
            hasData = false;
            imageStatus = IDLE;
        }
    }
}
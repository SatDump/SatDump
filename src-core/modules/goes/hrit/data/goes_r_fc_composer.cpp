#include "lrit_data_decoder.h"
#include "resources.h"
#include "imgui/imgui_image.h"
#include "logger.h"

namespace goes
{
    namespace hrit
    {
        GOESRFalseColorComposer::GOESRFalseColorComposer()
        {
            ch2_curve.load_png(resources::getResourcePath("goes/abi/wxstar/ch2_curve.png").c_str());
            fc_lut.load_png(resources::getResourcePath("goes/abi/wxstar/lut.png").c_str());

            time2 = 0;
            time13 = 0;

            imageStatus = IDLE;
        }

        GOESRFalseColorComposer::~GOESRFalseColorComposer()
        {
        }

        void GOESRFalseColorComposer::generateCompo()
        {
            imageStatus = RECEIVING;

            // Not really necessary, but good to be safe
            if (ch2.height() > 0)
                ch13.resize(ch2.width(), ch2.height());
            else
                ch2.resize(ch13.width(), ch13.height());

            falsecolor = image::Image<uint8_t>(ch2.width(), ch2.height(), 3); // Init image

            for (int i = 0; i < ch2.width() * ch2.height(); i++)
            {
                uint8_t x = ch2_curve[ch2[i]];
                uint8_t y = (ch13[i]);
                for (int c = 0; c < 3; c++)
                    falsecolor[c * falsecolor.width() * falsecolor.height() + i] = fc_lut[c * fc_lut.width() * fc_lut.height() + x * fc_lut.width() + y];
            }

            hasData = true;

            // If the UI is active, update texture
            if (textureID > 0)
            {
                // Downscale image
                img_height = 1000;
                img_width = 1000;
                image::Image<uint8_t> imageScaled = falsecolor;
                imageScaled.resize(img_width, img_height);
                uchar_to_rgba(imageScaled.data(), textureBuffer, img_height * img_width, 3);
                hasToUpdate = true;
            }
        }

        void GOESRFalseColorComposer::push2(image::Image<uint8_t> img, time_t time)
        {
            ch2 = img;
            time2 = time;

            if (time2 == time13)
                generateCompo();
        }

        void GOESRFalseColorComposer::push13(image::Image<uint8_t> img, time_t time)
        {
            ch13 = img;
            time13 = time;

            if (time2 == time13)
                generateCompo();
        }

        void GOESRFalseColorComposer::save(std::string directory)
        {
            imageStatus = SAVING;
            logger->info("Writing image " + directory + "/IMAGES/" + filename + ".png" + "...");
            logger->warn("This false color LUT was made by Harry Dove-Robinson, see resources/goes/abi/wxstar/README.md for more infos.");
            falsecolor.save_png(std::string(directory + "/IMAGES/" + filename + ".png").c_str());
            hasData = false;
            imageStatus = IDLE;
        }
    }
}
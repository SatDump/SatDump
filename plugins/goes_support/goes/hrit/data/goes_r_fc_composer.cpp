#include "lrit_data.h"
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
            if (time2 != time13)
                return;

            size_t width = ch2->width();
            size_t height = ch2->height();
            size_t lut_width = fc_lut.width();
            size_t total_size = width * height;
            size_t total_lut_size = lut_width * fc_lut.height();

            imageStatus = RECEIVING;
            if(ch13->height() != height)
            {
                std::shared_ptr<image::Image<uint8_t>> full_ch13 = ch13;
                ch13 = std::make_shared<image::Image<uint8_t>>(*full_ch13);
                ch13->resize(width, height);
            }

            if(!hasData)
                 falsecolor.init(width, height, 3);

            uint8_t *ch2_ptr = ch2->data();
            uint8_t *ch13_ptr = ch13->data();

            for (size_t i = 0; i < total_size; i++)
                for (int c = 0; c < 3; c++)
                    falsecolor[c * total_size + i] = fc_lut[c * total_lut_size + ch2_curve[ch2_ptr[i]] * lut_width + ch13_ptr[i]];

            hasData = true;

            // If the UI is active, update texture
            if (textureID > 0)
            {
                // Downscale image
                image::Image<uint8_t> imageScaled = falsecolor.resize_to(1000, 1000);
                uchar_to_rgba(imageScaled.data(), textureBuffer, 1000 * 1000, 3);
                hasToUpdate = true;
            }
        }

        void GOESRFalseColorComposer::push2(std::shared_ptr<image::Image<uint8_t>> const &img, time_t time)
        {
            if (time2 != 0 && time2 != time)
                save();

            //Keep Composer in sync - purge old data from composites where no data
            //Was received on one of the channels
            if (time13 != 0 && time > time13)
                time13 = 0;

            ch2 = img;
            time2 = time;
            generateCompo();
        }

        void GOESRFalseColorComposer::push13(std::shared_ptr<image::Image<uint8_t>> const &img, time_t time)
        {
            if (time13 != 0 && time13 != time)
                save();

            //Keep Composer in sync - purge old data from composites where no data
            //Was received on one of the channels
            if (time2 != 0 && time > time2)
                time2 = 0;

            ch13 = img;
            time13 = time;
            generateCompo();
        }

        void GOESRFalseColorComposer::save()
        {
            if (!hasData)
                return;

            imageStatus = SAVING;
            logger->info("This false color LUT was made by Harry Dove-Robinson, see resources/goes/abi/wxstar/README.md for more infos.");
            ch2.reset();
            ch13.reset();
            falsecolor.save_img(std::string(directory + "/IMAGES/" + filename).c_str());
            falsecolor.clear();
            hasData = false;
            time2 = time13 = 0;
            imageStatus = IDLE;
        }
    }
}

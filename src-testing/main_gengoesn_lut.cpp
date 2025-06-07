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

#include "image/brightness_contrast.h"
#include "image/hue_saturation.h"
#include "image/image.h"
#include "image/io.h"
#include "logger.h"
#include "core/resources.h"

image::Image applyLut(image::Image image3, image::Image image5)
{
    // If we can, generate false color
    if (resources::resourceExists("lut/goes/gvar/lut.png"))
    {
        //     logger->trace("Scale Ch1 to 8-bits...");
        image::Image channel1(8, image5.width(), image5.height(), 1);
        for (size_t i = 0; i < channel1.width() * channel1.height(); i++)
            channel1.set(i, image5.get(i) / 255);
        image5.clear(); // We're done with Ch1. Free up memory

        //    logger->trace("Scale Ch4 to 8-bits...");
        image::Image channel4(8, image3.width(), image3.height(), 1);
        for (size_t i = 0; i < channel4.width() * channel4.height(); i++)
            channel4.set(i, image3.get(i) / 255);
        image3.clear(); // We're done with Ch4. Free up memory

        //    logger->trace("Resize images...");
        channel4.resize(channel1.width(), channel1.height());

        //     logger->trace("Loading LUT...");
        image::Image lutImage;
        image::load_png(lutImage, resources::getResourcePath("lut/goes/gvar/lut.png").c_str());
        lutImage.resize(256, 256);

        //    logger->trace("Loading correction curve...");
        image::Image curveImage;
        image::load_png(curveImage, resources::getResourcePath("lut/goes/gvar/curve_goesn.png").c_str());

        image::Image compoImage(8, channel1.width(), channel1.height(), 3);

        //   logger->trace("Applying LUT...");
        for (size_t i = 0; i < channel1.width() * channel1.height(); i++)
        {
            uint8_t x = 255 - curveImage.get(channel1.get(i)) / 1.5;
            uint8_t y = channel4.get(i);

            for (int c = 0; c < 3; c++)
                compoImage.set(c * compoImage.width() * compoImage.height() + i, lutImage.get(c * lutImage.width() * lutImage.height() + x * lutImage.width() + y));
        }

        //  logger->trace("Contrast correction...");
        image::brightness_contrast(compoImage, -10.0f / 127.0f, 24.0f / 127.0f);

        //   logger->trace("Hue shift...");
        image::HueSaturation hueTuning;
        hueTuning.hue[image::HUE_RANGE_MAGENTA] = 133.0 / 180.0;
        hueTuning.overlap = 100.0 / 100.0;
        image::hue_saturation(compoImage, hueTuning);
        return compoImage;
    }
    else
    {
        logger->warn("lut/goes/gvar/lut.png LUT is missing! False Color will not be generated");
    }
}

int main(int argc, char *argv[])
{
    initLogger();

    image::Image finalLut(8, 256, 256, 3);

    for (int ch1_val = 0; ch1_val < 256; ch1_val++)
    {
        logger->critical(ch1_val);
        for (int ch4_val = 0; ch4_val < 256; ch4_val++)
        {
            image::Image ch1(16, 2, 2, 1);
            ch1.fill(ch1_val << 8);

            image::Image ch4(16, 2, 2, 1);
            ch4.fill(ch4_val << 8);

            auto out = applyLut(ch4, ch1);

            finalLut.set(0, ch1_val, ch4_val, out.get(0, 0, 0));
            finalLut.set(1, ch1_val, ch4_val, out.get(1, 0, 0));
            finalLut.set(2, ch1_val, ch4_val, out.get(2, 0, 0));
        }
    }

    image::save_png(finalLut, "/tmp/fc_lut.png");

    return 0;
}
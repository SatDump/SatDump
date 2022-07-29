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

int clamp(int x)
{
    if (x > 255)
        return 255;
    if (x < 0)
        return 0;
    return x;
}

int main(int argc, char *argv[])
{
    initLogger();

    image::Image<uint16_t> channel1, channel2, channel3;
    channel1.load_png(argv[1]);
    channel2.load_png(argv[2]);
    channel3.load_png(argv[3]);

    channel2.resize(channel1.width(), channel1.height());
    channel3.resize(channel1.width(), channel1.height());

    image::Image<uint8_t> natural_color;
    natural_color.init(channel1.width(), channel1.height(), 3);

    // channel1.white_balance();
    // channel2.white_balance();
    // channel3.white_balance();

    for (size_t i = 0; i < channel1.width() * channel1.height(); i++)
    {
#if 1
        natural_color.channel(0)[i] = clamp(sqrt(channel2[i]) * 0.8);
        natural_color.channel(1)[i] = clamp(sqrt(std::max<int>(0, std::min<int>(65535, 0.45 * channel2[i] + 0.1 * channel3[i] + 0.45 * channel1[1]))));
        natural_color.channel(2)[i] = clamp(sqrt(channel1[i]) * 0.8);

        for (int y = 0; y < 3; y++)
            natural_color.channel(y)[i] = clamp((natural_color.channel(y)[i] - 30) * 1.6);

#else
        natural_color.channel(0)[i] = channel2[i] >> 8;
        natural_color.channel(1)[i] = std::max<int>(0, std::min<int>(65535, 0.45 * channel2[i] + 0.1 * channel3[i] + 0.45 * channel1[1])) >> 8;
        natural_color.channel(2)[i] = channel1[i] >> 8;
#endif
    }

    natural_color.save_png(argv[4]);
}
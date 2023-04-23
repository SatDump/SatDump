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

// #include "common/image/image.h"
#include "common/dsp/filter/firdes.h"

#include "common/dsp/window/window.h"

int main(int argc, char *argv[])
{
    initLogger();

    std::vector<float> taps = /* dsp::windowed_sinc(361, 4, dsp::window::nuttall); //*/
        dsp::firdes::root_raised_cosine(1.0, 2, 1, 0.6, 361);
    // dsp::firdes::design_resampler_filter_float(2, 1, 0.2);

    //  image::Image<uint8_t> plot(taps.size(), taps.size(), 1);

    std::string array;

    array += "[";

    // uint8_t color[] = {255};
    for (int x = 0; x < taps.size(); x++)
    {
        if (x < taps.size() - 1)
            array += std::to_string(taps[x]) + ", ";
        else
            array += std::to_string(taps[x]);
        //  plot.draw_line(x, 0, x, taps[x] * taps.size() * 50, color);
    }
    // plot.mirror(false, true);
    // plot.save_img("taps_plot.png");

    array += "]";

    std::string command = "octave --eval \'taps = " + array + "\n freqz(taps)\n\' --persist";

    // printf("%s\n", command.c_str());
    system(command.c_str());
}

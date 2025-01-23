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
#include "common/image/equation.h"
#include "common/image/io.h"

int main(int argc, char *argv[])
{
    initLogger();

    image::Image img321;
    image::load_img(img321, "/home/alan/Downloads/testimgsave.jpg");

    std::vector<image::EquationChannel> ch;
    ch.push_back({"ch3,ch2,ch1", &img321});
    auto img2 = image::generate_image_equation(ch, "ch2 * ch3, ch2 * ch3, ch1 * ch3");

    image::save_img(img2, "/home/alan/Downloads/testimgsave_NEWEQU.jpg");
}

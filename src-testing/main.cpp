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

#include <fstream>
#include "common/image/image.h"

int main(int argc, char *argv[])
{
    initLogger();

    std::ifstream data_in(argv[1], std::ios::binary);
    int size = std::stoi(argv[3]);

    uint16_t *buffer = new uint16_t[size * size];

    data_in.read((char *)buffer, sizeof(uint16_t) * size * size);

    image::Image<uint16_t> img(buffer, size, size, 1);

    img.save_png(argv[2]);
}

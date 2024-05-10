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

#include "common/tile_map/map.h"
#include "common/image/io.h"

int main(int argc, char *argv[])
{
    initLogger();
    completeLoggerInit();

    image::Image img = downloadTileMap("https://tile.openstreetmap.org/{z}/{x}/{y}.png", /*48.7, 1.7, 48.9, 1.9, 17); */ -85.0511, -180, 85.0511, 180, 4);
    image::save_tiff(img, argv[1]);
}
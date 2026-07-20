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

#include "common/codings/randomization.h"
#include "common/net/udp.h"
#include "common/tile_map/map.h"
#include "dsp/benchmark/bench.h"
#include "handlers/vector/shapefile_handler.h"
#include "image/meta.h"
#include "image/spectral_align.h"
#include "init.h"
#include "logger.h"

#include "common/ccsds/ccsds_tm/demuxer.h"
#include "common/ccsds/ccsds_tm/vcdu.h"
#include "common/simple_deframer.h"
#include <cstdint>
#include <cstdlib>
#include <fstream>

#include "image/image.h"
#include "image/io.h"
#include "image/processing.h"

#include <cstring>

#include "common/repack.h"

#include "image/bayer/bayer.h"

#include "common/codings/reedsolomon/reedsolomon.h"
#include "projection/projection.h"

int main(int argc, char *argv[])
{
    initLogger();

    logger->set_level(slog::LOG_OFF);
    satdump::initSatDump();
    completeLoggerInit();
    logger->set_level(slog::LOG_TRACE);

    auto img = downloadTileMap("https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}", //
                               38.0726974 + 0.12, 53.8183079 + 0.20, //                                                                          //
                               38.0726974 - 0.02, 53.8183079 - 0.10, //
                               14);

    auto bpk = img;
    img.init(8, bpk.width(), bpk.height(), 1);
    for (size_t i = 0; i < bpk.height() * bpk.width(); i++)
        img.setf(i, (bpk.getf(0, i) + bpk.getf(1, i) + bpk.getf(2, i)) / 3.0);

    image::save_img(img, "/home/alan/Downloads/idk.png");
}

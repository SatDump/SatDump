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

#include "db/kepler/kepler_handler.h"
#include "image/image.h"
#include "image/io.h"
#include "logger.h"
#include <cstdint>
#include <cstring>

namespace satdump
{
    std::vector<KeplerData> get_tle_simple_url_group();
}

int main(int argc, char *argv[]) { initLogger(); }
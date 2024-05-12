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
#include "common/projection/reprojector.h"
#include "nlohmann/json_utils.h"

int main(int argc, char *argv[])
{
    initLogger();
    completeLoggerInit();

    nlohmann::json proj_cfg = loadJsonFile(argv[1]);
    int width = std::stoi(argv[2]);
    int height = std::stoi(argv[3]);

    auto proj_func = satdump::reprojection::setupProjectionFunction(width,
                                                                    height,
                                                                    proj_cfg);

    auto p = proj_func(30, -90, width, height);

    logger->trace("%f %f", p.first, p.second);
}
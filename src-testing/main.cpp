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

#include "init.h"
#include "common/tracking/tle.h"

int main(int argc, char *argv[])
{
    initLogger();
    // We don't wanna spam with init this time around
    logger->set_level(slog::LOG_OFF);
    satdump::initSatdump();
    completeLoggerInit();
    logger->set_level(slog::LOG_TRACE);

    auto tle = satdump::general_tle_registry.get_from_norad_time(40069, time(0) - 24 * 3600 * 100);

    if (tle.has_value())
        logger->trace(tle.value().name);
}
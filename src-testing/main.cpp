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

#include "init.h"
#include "logger.h"

#include "utils/file/file_iterators.h"
#include "utils/file/folder_file_iterators.h"
#include "utils/file/zip_file_iterators.h"
#include "utils/time.h"
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

int main(int argc, char *argv[])
{
    initLogger();

    logger->set_level(slog::LOG_OFF);
    satdump::initSatdump();
    completeLoggerInit();
    logger->set_level(slog::LOG_TRACE);
}
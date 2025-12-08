#include "core/cli/cli.h"
#include "init.h"
#include "logger.h"
#include <memory>

int main(int argc, char *argv[])
{
    // Init logger
    initLogger();

    // Basic flags
    bool verbose = satdump::cli::checkVerbose(argc, argv);

    // Init SatDump, silent or verbose as requested
    if (!verbose)
        logger->set_level(slog::LOG_WARN);
    satdump::initSatdump();
    completeLoggerInit();
    if (!verbose)
        logger->set_level(slog::LOG_TRACE);

    return satdump::cli::handleCommand(argc, argv);
}

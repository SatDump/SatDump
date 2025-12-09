#include "core/cli/cli.h"
#include "core/plugin.h"
#include "init.h"
#include "logger.h"
#include "old.h"
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

    satdump::eventBus->register_handler<satdump::cli::RegisterSubcommandEvent>([](const satdump::cli::RegisterSubcommandEvent &evt)
                                                                               { evt.cmd_handlers.push_back(std::make_shared<satdump::OldCmdHandler>()); });

    satdump::cli::CommandHandler h;
    int v = h.parse(argc, argv);
    if (v != 0)
        return v;
    h.run();
    return 0;
}

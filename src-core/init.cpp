#include "init.h"
#include "logger.h"
#include "module.h"
#include "pipeline.h"

void initSatdump()
{
    logger->info("   _____       __  ____                      ");
    logger->info("  / ___/____ _/ /_/ __ \\__  ______ ___  ____ ");
    logger->info("  \\__ \\/ __ `/ __/ / / / / / / __ `__ \\/ __ \\");
    logger->info(" ___/ / /_/ / /_/ /_/ / /_/ / / / / / / /_/ /");
    logger->info("/____/\\__,_/\\__/_____/\\__,_/_/ /_/ /_/ .___/ ");
    logger->info("                                    /_/      ");
    logger->info("Starting SatDump v1.0");
    logger->info("");

    registerModules();

    loadPipelines("pipelines");

    logger->debug("Registered pipelines :");
    for (Pipeline &pipeline : pipelines)
        logger->debug(" - " + pipeline.name);
}
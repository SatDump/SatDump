#include "init.h"
#include "logger.h"
#include "module.h"
#include "pipeline.h"
#include <filesystem>
#include "settings.h"
#include "tle.h"
#include "plugin.h"
#include "satdump_vars.h"

void initSatdump()
{
    logger->info("   _____       __  ____                      ");
    logger->info("  / ___/____ _/ /_/ __ \\__  ______ ___  ____ ");
    logger->info("  \\__ \\/ __ `/ __/ / / / / / / __ `__ \\/ __ \\");
    logger->info(" ___/ / /_/ / /_/ /_/ / /_/ / / / / / / /_/ /");
    logger->info("/____/\\__,_/\\__/_____/\\__,_/_/ /_/ /_/ .___/ ");
    logger->info("                                    /_/      ");
    logger->info("Starting SatDump v" + (std::string)SATDUMP_VERSION);
    logger->info("");

    loadConfig();

    loadPlugins();

    registerModules();

    // Load pipelines
    if (std::filesystem::exists("pipelines") && std::filesystem::is_directory("pipelines"))
        loadPipelines("pipelines");
    else
        loadPipelines((std::string)RESOURCES_PATH + "pipelines");

    // List them
    logger->debug("Registered pipelines :");
    for (Pipeline &pipeline : pipelines)
        logger->debug(" - " + pipeline.name);

    // TLEs
    tle::loadTLEs();

    // Let plugins know we started
    satdump::eventBus->fire_event<satdump::SatDumpStartedEvent>({});
}

// This is a pretty crappy way of doing it,
// but it avoids a few libraries complaining
// when linking in the Android NDK
#ifdef __ANDROID__
#include <stdio.h>
extern "C"
{
#undef stderr
    FILE *stderr = NULL;
#undef stdout
    FILE *stdout = NULL;
}
#endif
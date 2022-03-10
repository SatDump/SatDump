#include "init.h"
#include "logger.h"
#include "core/module.h"
#include "core/pipeline.h"
#include <filesystem>
//#include "tle.h"
#include "core/plugin.h"
#include "satdump_vars.h"
#include "core/config.h"

namespace satdump
{
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

        config::loadConfig("satdump_cfg.json");

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
        // tle::loadTLEs();

        // Let plugins know we started
        eventBus->fire_event<SatDumpStartedEvent>({});
    }
}
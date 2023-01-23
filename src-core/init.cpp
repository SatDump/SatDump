#define SATDUMP_DLL_EXPORT 1
#include "init.h"
#include "logger.h"
#include "core/module.h"
#include "core/pipeline.h"
#include <filesystem>
// #include "tle.h"
#include "core/plugin.h"
#include "satdump_vars.h"
#include "core/config.h"

#include "common/tracking/tle.h"
#include "products/products.h"

#include "core/opencl.h"

namespace satdump
{
    SATDUMP_DLL std::string user_path;
    SATDUMP_DLL std::string tle_file_override = "";
    SATDUMP_DLL bool tle_do_update_on_init = true;

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

#ifdef _WIN32
        user_path = ".";
#elif __ANDROID__
        user_path = ".";
#else
        user_path = std::string(getenv("HOME")) + "/.config/satdump";
#endif

        if (std::filesystem::exists("satdump_cfg.json"))
            config::loadConfig("satdump_cfg.json", user_path);
        else
            config::loadConfig(satdump::RESPATH + "satdump_cfg.json", user_path);

        if (config::main_cfg["satdump_general"].contains("log_to_file"))
        {
            bool log_file = config::main_cfg["satdump_general"]["log_to_file"]["value"];
            if (log_file)
                initFileSink();
        }

        if (config::main_cfg["satdump_general"].contains("log_level"))
        {
            std::string log_level = config::main_cfg["satdump_general"]["log_level"]["value"];
            if (log_level == "trace")
                setConsoleLevel(spdlog::level::level_enum::trace);
            else if (log_level == "debug")
                setConsoleLevel(spdlog::level::level_enum::debug);
            else if (log_level == "info")
                setConsoleLevel(spdlog::level::level_enum::info);
            else if (log_level == "warn")
                setConsoleLevel(spdlog::level::level_enum::warn);
            else if (log_level == "error")
                setConsoleLevel(spdlog::level::level_enum::err);
            else if (log_level == "critical")
                setConsoleLevel(spdlog::level::level_enum::critical);
        }

        loadPlugins();

        registerModules();

        // Load pipelines
        if (std::filesystem::exists("pipelines") && std::filesystem::is_directory("pipelines"))
            loadPipelines("pipelines");
        else
            loadPipelines(satdump::RESPATH + "pipelines");

        // List them
        logger->debug("Registered pipelines :");
        for (Pipeline &pipeline : pipelines)
            logger->debug(" - " + pipeline.name);

#ifdef USE_OPENCL
        // OpenCL
        opencl::initOpenCL();
#endif

        // TLEs
        if (tle_file_override == "")
        {
            if (config::main_cfg["satdump_general"]["update_tles_startup"]["value"].get<bool>() && tle_do_update_on_init)
                updateTLEFile(user_path + "/satdump_tles.txt");
            loadTLEFileIntoRegistry(user_path + "/satdump_tles.txt");
            if (general_tle_registry.size() == 0 && tle_do_update_on_init) // NO TLEs? Download now.
            {
                updateTLEFile(user_path + "/satdump_tles.txt");
                loadTLEFileIntoRegistry(user_path + "/satdump_tles.txt");
            }
        }
        else
        {
            if (std::filesystem::exists(tle_file_override))
                loadTLEFileIntoRegistry(tle_file_override);
            else
                logger->error("TLE File doesn't exist : " + tle_file_override);
        }

        // Products
        registerProducts();

        // Let plugins know we started
        eventBus->fire_event<SatDumpStartedEvent>({});

#ifdef BUILD_IS_DEBUG
        // If in debug, warn the user!
        logger->error("██████╗  █████╗ ███╗   ██╗ ██████╗ ███████╗██████╗ ");
        logger->error("██╔══██╗██╔══██╗████╗  ██║██╔════╝ ██╔════╝██╔══██╗");
        logger->error("██║  ██║███████║██╔██╗ ██║██║  ███╗█████╗  ██████╔╝");
        logger->error("██║  ██║██╔══██║██║╚██╗██║██║   ██║██╔══╝  ██╔══██╗");
        logger->error("██████╔╝██║  ██║██║ ╚████║╚██████╔╝███████╗██║  ██║");
        logger->error("╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚══════╝╚═╝  ╚═╝");
        logger->error("SatDump has NOT been built in Release mode.");
        logger->error("If you are not a developer but intending to use the software,");
        logger->error("you probably do not want this. If so, make sure to");
        logger->error("specify -DCMAKE_BUILD_TYPE=Release in CMake.");
#endif
    }
}
#define SATDUMP_DLL_EXPORT 1

#include "pipeline/module.h"
#include "pipeline/pipeline.h"

#include "core/config.h"
#include "core/plugin.h"
#include "core/resources.h"
#include "init.h"
#include "logger.h"
#include "satdump_vars.h"
#include <filesystem>

#include "common/tracking/tle.h"

#include "core/opencl.h"

#include "common/dsp/buffer.h"

#include "products/product.h"

namespace satdump
{
    SATDUMP_DLL std::string user_path;
    SATDUMP_DLL std::string tle_file_override = "";
    SATDUMP_DLL bool tle_do_update_on_init = true;

    void initSatdump(bool is_gui)
    {
        logger->info("   _____       __  ____                      ");
        logger->info("  / ___/____ _/ /_/ __ \\__  ______ ___  ____ ");
        logger->info("  \\__ \\/ __ `/ __/ / / / / / / __ `__ \\/ __ \\");
        logger->info(" ___/ / /_/ / /_/ /_/ / /_/ / / / / / / /_/ /");
        logger->info("/____/\\__,_/\\__/_____/\\__,_/_/ /_/ /_/ .___/ ");
        logger->info("                                    /_/      ");
        logger->info("Starting " + getSatDumpVersionName());
        logger->info("");

#ifdef _WIN32
        if (std::filesystem::exists("satdump_cfg.json"))
            user_path = "./config";
        else
            user_path = std::string(getenv("APPDATA")) + "/satdump";
#elif __ANDROID__
        user_path = ".";
#else
        user_path = std::string(getenv("HOME")) + "/.config/satdump";
#endif

        try
        {
            if (std::filesystem::exists("satdump_cfg.json"))
                satdump_cfg.load("satdump_cfg.json", user_path);
            else
                satdump_cfg.load(satdump::RESPATH + "satdump_cfg.json", user_path);
        }
        catch (std::exception &e)
        {
            logger->critical("Error loading SatDump config! SatDump will now exit. Error:\n%s", e.what());
            // if (is_gui)
            //    pfd::message("SatDump", "Error loading SatDump config! SatDump will now exit. Error:\n\n" + std::string(e.what()), pfd::choice::ok, pfd::icon::error); TODOREWORK bring this back
            exit(1);
        }

        if (satdump_cfg.main_cfg["satdump_general"].contains("log_to_file"))
        {
            bool log_file = satdump_cfg.main_cfg["satdump_general"]["log_to_file"]["value"];
            if (log_file)
                initFileSink();
        }

        if (satdump_cfg.main_cfg["satdump_general"].contains("log_level"))
        {
            std::string log_level = satdump_cfg.main_cfg["satdump_general"]["log_level"]["value"];
            if (log_level == "trace")
                setConsoleLevel(slog::LOG_TRACE);
            else if (log_level == "debug")
                setConsoleLevel(slog::LOG_DEBUG);
            else if (log_level == "info")
                setConsoleLevel(slog::LOG_INFO);
            else if (log_level == "warn")
                setConsoleLevel(slog::LOG_WARN);
            else if (log_level == "error")
                setConsoleLevel(slog::LOG_ERROR);
            else if (log_level == "critical")
                setConsoleLevel(slog::LOG_CRIT);
        }

        loadPlugins();

        // Let plugins know we started
        satdump_cfg.registerPlugins();

        // TODOREWORK
        pipeline::registerModules();

        // Load pipelines
        // TODOREWORK
        // loadPipelines(resources::getResourcePath("pipelines"));
        pipeline::loadPipelines(resources::getResourcePath("pipelines"));

        // List them
        logger->debug("Registered pipelines :");
        for (auto &pipeline : pipeline::pipelines)
            logger->debug(" - " + pipeline.id);

#ifdef USE_OPENCL
        // OpenCL
        opencl::initOpenCL();
#endif

        // TLEs
        if (tle_file_override == "")
        {
            loadTLEFileIntoRegistry(user_path + "/satdump_tles.txt");
            if (tle_do_update_on_init)
                autoUpdateTLE(user_path + "/satdump_tles.txt");
        }
        else
        {
            if (std::filesystem::exists(tle_file_override))
                loadTLEFileIntoRegistry(tle_file_override);
            else
                logger->error("TLE File doesn't exist : " + tle_file_override);
        }

        // Products
        products::registerProducts();

        // Set DSP buffer sizes if they have been changed TODOREWORK remove this!
        if (satdump_cfg.main_cfg.contains("advanced_settings"))
        {
            if (satdump_cfg.main_cfg["advanced_settings"].contains("default_buffer_size"))
            {
                int new_sz = satdump_cfg.main_cfg["advanced_settings"]["default_buffer_size"].get<int>();
                dsp::STREAM_BUFFER_SIZE = new_sz;
                dsp::RING_BUF_SZ = new_sz;
                logger->warn("DSP Buffer size was changed to %d", new_sz);
            }
        }

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
} // namespace satdump
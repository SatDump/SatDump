#define SATDUMP_DLL_EXPORT 1
#include "init.h"
#include "core/config.h"
#include "core/plugin.h"
#include "core/resources.h"
#include "logger.h"
#include "satdump_vars.h"
#include <filesystem>

#include "db/db_handler.h"
#include "db/iers/iers_handler.h"
#include "db/tle/tle_handler.h"
#include <memory>

#include "pipeline/module.h"
#include "pipeline/pipeline.h"

#include "common/tracking/tle.h"

#include "core/opencl.h"

#include "common/dsp/buffer.h"

#include "products/product.h"

#include "utils/webhook.h"

// TODOREWORK?
extern "C"
{
#include "libs/supernovas/novas.h"

#include "libs/calceph/calceph.h"
#include "libs/supernovas/novas-calceph.h"
}

namespace satdump
{
    SATDUMP_DLL std::string user_path;
    SATDUMP_DLL std::string tle_file_override = "";
    SATDUMP_DLL bool tle_do_update_on_init = true;

    SATDUMP_DLL std::shared_ptr<DBHandler> db;
    SATDUMP_DLL std::shared_ptr<TleDBHandler> db_tle;
    SATDUMP_DLL std::shared_ptr<IersDBHandler> db_iers;

    void initSatDump(bool is_gui)
    {
        auto lvl = logger->get_level();
        logger->set_level(slog::LOG_INFO);
        logger->info("   _____       __  ____                      ");
        logger->info("  / ___/____ _/ /_/ __ \\__  ______ ___  ____ ");
        logger->info("  \\__ \\/ __ `/ __/ / / / / / / __ `__ \\/ __ \\");
        logger->info(" ___/ / /_/ / /_/ /_/ / /_/ / / / / / / /_/ /");
        logger->info("/____/\\__,_/\\__/_____/\\__,_/_/ /_/ /_/ .___/ ");
        logger->info("                                    /_/      ");
        logger->info("Starting " + getSatDumpVersionName());
        logger->info("");
        logger->set_level(lvl);

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

            db = std::make_shared<DBHandler>(user_path + "/main.db");
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

        // Database : TLEs, IERS stuff, etc...
        db->subhandlers.push_back(db_tle = std::make_shared<TleDBHandler>(db));
        db->subhandlers.push_back(db_iers = std::make_shared<IersDBHandler>(db));
        db->init();

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

        // Init SuperNOVAS/CalCEPH. TODOREWORK, as we need to be able to dynamically load them!
        std::string de440_f = resources::getResourcePath("spice/de440s.bsp");
        const char *spice_kernels[] = {de440_f.c_str()};
        t_calcephbin *de440 = calceph_open_array(1, spice_kernels); //// calceph_open("/home/alan/Downloads/de440s.bsp");
        if (!de440)
            logger->error("Could not open ephemeris data! NOVAS will not work!");
        novas_use_calceph(de440);

        // Init webhook event handler if URL set in config
        std::string webhook_url = satdump_cfg.getValueFromSatDumpGeneral<std::string>("webhook_url");
        if (webhook_url != "")
            satdump::Webhook webhook;

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

        // Start task scheduler
        taskScheduler->start_thread();
    }

    void exitSatDump()
    {
        logger->info("Exiting SatDump! Bye!");
        taskScheduler.reset();
    }
} // namespace satdump
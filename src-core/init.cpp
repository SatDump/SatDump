#include "core/style.h"
#define SATDUMP_DLL_EXPORT 1

#include "db/kepler/kepler_handler.h"
#include "i18n.h"
#include <cstdlib>

#include "core/config.h"
#include "core/plugin.h"
#include "core/resources.h"
#include "init.h"
#include "logger.h"
#include "satdump_vars.h"
#include <filesystem>

#include "db/db_handler.h"
#include "db/iers/iers_handler.h"
#include <memory>

#include "pipeline/module.h"
#include "pipeline/pipeline.h"

#include "common/tracking/tle.h"

#include "core/opencl.h"

#include "common/dsp/buffer.h"

#include "products/product.h"

#if defined(_WIN32)
#include <Windows.h>
#endif

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
    SATDUMP_DLL std::shared_ptr<KeplerDBHandler> db_keplers;
    SATDUMP_DLL std::shared_ptr<IersDBHandler> db_iers;

#if ENABLE_I18N
    void initLanguage(std::string lang)
    {
#if defined(_WIN32)
        SetEnvironmentVariable("LC_NUMERIC", "C");
#else
        setenv("LC_NUMERIC", "C", true);
#endif

        if (lang.size())
#if defined(_WIN32)
            SetEnvironmentVariable("LANGUAGE", lang.c_str());
#else
            setenv("LANGUAGE", lang.c_str(), true);
#endif
        else
#if defined(_WIN32)
            SetEnvironmentVariable("LANGUAGE", ""); // TODOREWORK check
#else
            unsetenv("LANGUAGE");
#endif

        setlocale(LC_ALL, "");
        bindtextdomain("satdump", resources::getResourcePath("i18n").c_str());
        textdomain("satdump");

        std::string old_val = style::i18n_extraFont;
        std::string i18n_lang_extra_font(_("i18n_lang_extra_font"));
        if (i18n_lang_extra_font != "i18n_lang_extra_font")
            style::i18n_extraFont = i18n_lang_extra_font;
        else
            style::i18n_extraFont = "";

        if (style::i18n_extraFont != old_val)
            eventBus->fire_event<StyleOrUINeedUpdateEvent>({});

        current_language = lang;
    }

    SATDUMP_DLL std::string current_language;
#endif

    void initSatDump(bool is_gui)
    {
#if ENABLE_I18N
        initLanguage();
#endif

        auto lvl = logger->get_level();
        logger->set_level(slog::LOG_INFO);
        logger->info("   _____       __  ____                      ");
        logger->info("  / ___/____ _/ /_/ __ \\__  ______ ___  ____ ");
        logger->info("  \\__ \\/ __ `/ __/ / / / / / / __ `__ \\/ __ \\");
        logger->info(" ___/ / /_/ / /_/ /_/ / /_/ / / / / / / /_/ /");
        logger->info("/____/\\__,_/\\__/_____/\\__,_/_/ /_/ /_/ .___/ ");
        logger->info("                                    /_/      ");
        logger->info(_("Starting ") + getSatDumpVersionName());
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
            logger->critical(_("Error loading SatDump config! SatDump will now exit. Error:\n%s"), e.what());
            // if (is_gui)
            //    pfd::message("SatDump", "Error loading SatDump config! SatDump will now exit. Error:\n\n" + std::string(e.what()), pfd::choice::ok, pfd::icon::error); TODOREWORK bring this back
            exit(1);
        }

#if ENABLE_I18N
        std::string override_lang = db->get_user("language");
        if (override_lang != "")
            initLanguage(override_lang);
#endif

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
        logger->debug(_("Registered pipelines :"));
        for (auto &pipeline : pipeline::pipelines)
            logger->debug(" - " + pipeline.id);

#ifdef USE_OPENCL
        // OpenCL
        opencl::initOpenCL();
#endif

        // Database : TLEs, IERS stuff, etc...
        db->subhandlers.push_back(db_keplers = std::make_shared<KeplerDBHandler>(db));
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
                logger->warn(_("DSP Buffer size was changed to %d"), new_sz);
            }
        }

        // Init SuperNOVAS/CalCEPH. TODOREWORK, as we need to be able to dynamically load them!
        std::string de440_f = resources::getResourcePath("spice/de440s.bsp");
        const char *spice_kernels[] = {de440_f.c_str()};
        t_calcephbin *de440 = calceph_open_array(1, spice_kernels); //// calceph_open("/home/alan/Downloads/de440s.bsp");
        if (!de440)
            logger->error(_("Could not open ephemeris data! NOVAS will not work!"));
        novas_use_calceph(de440);

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
        logger->error(_("SatDump has NOT been built in Release mode."));
        logger->error(_("If you are not a developer but intending to use the software,"));
        logger->error(_("you probably do not want this. If so, make sure to"));
        logger->error(_("specify -DCMAKE_BUILD_TYPE=Release in CMake."));
#endif

        // Start task scheduler
        taskScheduler->start_thread();
    }

    void exitSatDump()
    {
        logger->info(_("Exiting SatDump! Bye!"));
        taskScheduler.reset();
    }
} // namespace satdump
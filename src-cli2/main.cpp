#include "CLI11.hpp"
#include "core/config.h"
#include "core/module.h"
#include "core/pipeline.h"
#include "init.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "satdump_vars.h"

int main(int argc, char *argv[])
{
    // Init logger
    initLogger();

    // Basic flags
    bool verbose = false;
    for (int i = 0; i < argc; i++)
        if (std::string(argv[i]) == "-v" || std::string(argv[i]) == "--verbose")
            verbose = true;

    // Init SatDump, silent (TODOREWORK, a verbose flag?)
    if (!verbose)
        logger->set_level(slog::LOG_WARN);
    satdump::initSatdump();
    completeLoggerInit();
    if (!verbose)
        logger->set_level(slog::LOG_TRACE);

    // TODOREWORK
    CLI::App app("SatDump v" + satdump::SATDUMP_VERSION);
    app.set_help_all_flag("--help-all", "Expand all help");
    app.add_flag("-v,--verbose", verbose, "Make the logger more verbose");
    app.require_subcommand();

    CLI::App *sub_run = app.add_subcommand("run", "Run a script");
    sub_run->add_flag("--lua", "Run a Lua script");
    sub_run->add_flag("--as", "Run an AngelScript script");

    CLI::App *sub_module = app.add_subcommand("module", "Run a single module");
    for (auto &p : modules_registry)
    {
        CLI::App *sub_p = sub_module->add_subcommand(p.first);
        sub_p->add_option("input_file");
        sub_p->add_option("output_hint");
        //  for (auto &ep : p.editable_parameters.items())
        //      sub_p->add_flag("--" + ep.key());
    }

    CLI::App *sub_pipeline = app.add_subcommand("pipeline", "Run a pipeline");
    for (auto &p : satdump::pipelines)
    {
        CLI::App *sub_p = sub_pipeline->add_subcommand(p.name);

        sub_p->add_option("level", "Level of the input file. Can be cadu, file, baseband...");
        sub_p->add_option("input_file", "Actual input file (eg. metop_ahrpt.cadu, a baseband, etc)");
        sub_p->add_option("output_folder", "Output folder for processed data");

        nlohmann::json common = satdump::config::main_cfg["user_interface"]["default_offline_parameters"];

        for (auto &ep : p.editable_parameters.items())
            for (auto &e : ep.value().items())
                common[ep.key()][e.key()] = p.editable_parameters[ep.key()][e.key()];

        for (auto &ep : common.items())
        {
            if (ep.value().contains("description"))
                sub_p->add_flag("--" + ep.key(), (const std::string)ep.value()["description"].get<std::string>());
            else
                sub_p->add_flag("--" + ep.key());
        }
    }

    CLI11_PARSE(app, argc, argv);

    for (auto *subcom : app.get_subcommands())
    {
        std::cout << "Subcommand: " << subcom->get_name() << '\n';
        if (subcom->get_name() == "pipeline")
        {
            for (auto *s2 : subcom->get_subcommands())
            {
                std::string t = s2->get_option("level")->get_option_text();
                logger->info(s2->get_name() + " level " + t);
            }
        }
    }
}

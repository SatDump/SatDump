#include "CLI11.hpp"
#include "common/detect_header.h"
#include "core/config.h"
#include "core/module.h"
#include "core/pipeline.h"
#include "init.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "satdump_vars.h"

#include "core/pipeline.h"
#include <memory>

#include "run_as.h"
#include "run_lua.h"

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
    sub_run->add_option("script", "The script to run")->required();
    auto sub_run_group = sub_run->add_option_group("--lua,--as");
    sub_run_group->add_flag("--lua", "Run a Lua script");
    sub_run_group->add_flag("--as", "Run an AngelScript script");
    sub_run_group->require_option(1);
    sub_run->add_flag("--lint", "Lint the script, without executing it");
    sub_run->add_flag("--predef", "Dump as.predefined");
    sub_run->require_option(2, 3);

    CLI::App *sub_module = app.add_subcommand("module", "Run a single module");
    for (auto &p : modules_registry)
    {
        CLI::App *sub_p = sub_module->add_subcommand(p.first);
        sub_p->add_option("input_file");
        sub_p->add_option("output_hint");
        //  for (auto &ep : p.editable_parameters.items())
        //      sub_p->add_flag("--" + ep.key());
    }

    std::map<std::string, std::map<std::string, std::shared_ptr<std::string>>> pipeline_opts;

    CLI::App *sub_pipeline = app.add_subcommand("pipeline", "Run a pipeline");
    for (auto &p : satdump::pipelines)
    {
        CLI::App *sub_p = sub_pipeline->add_subcommand(p.name);

        sub_p->add_option("level", "Level of the input file. Can be cadu, file, baseband...")->required();
        sub_p->add_option("input_file", "Actual input file (eg. metop_ahrpt.cadu, a baseband, etc)")->required();
        sub_p->add_option("output_folder", "Output folder for processed data")->required();

        nlohmann::json common = satdump::config::main_cfg["user_interface"]["default_offline_parameters"];

        for (auto &ep : p.editable_parameters.items())
            for (auto &e : ep.value().items())
                common[ep.key()][e.key()] = p.editable_parameters[ep.key()][e.key()];

        pipeline_opts.emplace(p.name, std::map<std::string, std::shared_ptr<std::string>>());

        for (auto &ep : common.items())
        {
            auto opt = std::make_shared<std::string>();
            if (ep.value().contains("description"))
                sub_p->add_flag("--" + ep.key(), *opt, (const std::string)ep.value()["description"].get<std::string>());
            else
                sub_p->add_flag("--" + ep.key(), *opt, "");
            pipeline_opts[p.name].emplace(ep.key(), opt);
        }
    }

    CLI11_PARSE(app, argc, argv);

    for (auto *subcom : app.get_subcommands())
    {
        //  std::cout << "Subcommand: " << subcom->get_name() << '\n';

        if (subcom->get_name() == "pipeline")
        {
            for (auto *s2 : subcom->get_subcommands())
            {
                std::string level = s2->get_option("level")->as<std::string>();
                std::string input = s2->get_option("input_file")->as<std::string>();
                std::string output = s2->get_option("output_folder")->as<std::string>();

                auto pipeline = satdump::getPipelineFromName(s2->get_name());

                nlohmann::json params;

                try_get_params_from_input_file(params, input);

                for (auto *s33 : s2->get_options())
                {
                    if (s2->count(s33->get_name()))
                    {
                        logger->critical(s33->get_name().substr(2));
                        auto optname = s33->get_name().substr(2);
                        if (pipeline_opts[pipeline->name].count(optname))
                            params[optname] = nlohmann::json::parse(s33->as<std::string>());
                    }
                }

                // logger->critical(params.dump());

                // Create output dir
                if (!std::filesystem::exists(output))
                    std::filesystem::create_directories(output);

                pipeline->run(input, output, params, level);
            }
        }
        else if (subcom->get_name() == "run")
        {
            if (subcom->count("--as"))
            {
                std::string script = subcom->get_option("script")->as<std::string>();
                satdump::runAngelScript(script, subcom->count("--lint"), subcom->count("--predef"));
            }
            else if (subcom->count("--lua"))
            {
                std::string script = subcom->get_option("script")->as<std::string>();
                satdump::runLua(script);
            }
        }
    }
}

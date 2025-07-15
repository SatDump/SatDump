#include "pipeline.h"
#include "common/detect_header.h"
#include "core/config.h"
#include "pipeline/pipeline.h"

namespace satdump
{
    void PipelineCmdHandler::reg(CLI::App *app)
    {
        CLI::App *sub_pipeline = app->add_subcommand("pipeline", "Run a pipeline");
        for (auto &p : satdump::pipeline::pipelines)
        {
            CLI::App *sub_p = sub_pipeline->add_subcommand(p.id);

            sub_p->add_option("level", "Level of the input file. Can be cadu, file, baseband...")->required();
            sub_p->add_option("input_file", "Actual input file (eg. metop_ahrpt.cadu, a baseband, etc)")->required();
            sub_p->add_option("output_folder", "Output folder for processed data")->required();

            nlohmann::json common = satdump::satdump_cfg.main_cfg["user_interface"]["default_offline_parameters"];

            for (auto &ep : p.editable_parameters.items())
                for (auto &e : ep.value().items())
                    common[ep.key()][e.key()] = p.editable_parameters[ep.key()][e.key()];

            pipeline_opts.emplace(p.id, std::map<std::string, std::shared_ptr<std::string>>());

            for (auto &ep : common.items())
            {
                auto opt = std::make_shared<std::string>();
                CLI::Option *f;
                if (ep.value().contains("description"))
                    f = sub_p->add_flag("--" + ep.key(), *opt, (const std::string)ep.value()["description"].get<std::string>());
                else
                    f = sub_p->add_flag("--" + ep.key(), *opt, "");
                if (ep.value().contains("value"))
                {
                    if (ep.value()["value"].is_string())
                        f->default_val(ep.value()["value"].get<std::string>());
                    else
                        f->default_val(ep.value()["value"].dump());
                }
                pipeline_opts[p.id].emplace(ep.key(), opt);
            }
        }
    }

    void PipelineCmdHandler::run(CLI::App *app, CLI::App *subcom)
    {
        for (auto *s2 : subcom->get_subcommands())
        {
            std::string level = s2->get_option("level")->as<std::string>();
            std::string input = s2->get_option("input_file")->as<std::string>();
            std::string output = s2->get_option("output_folder")->as<std::string>();

            auto pipeline = satdump::pipeline::getPipelineFromID(s2->get_name());

            nlohmann::json params;

            try_get_params_from_input_file(params, input);

            for (auto *s33 : s2->get_options())
            {
                if (s2->count(s33->get_name()))
                {
                    // logger->critical(s33->get_name().substr(2));
                    auto optname = s33->get_name().substr(2);
                    if (pipeline_opts[pipeline.id].count(optname))
                    {
                        try
                        {
                            params[optname] = nlohmann::json::parse(s33->as<std::string>());
                        }
                        catch (std::exception &e)
                        {
                            params[optname] = s33->as<std::string>();
                        }
                    }
                }
            }

            // logger->critical(params.dump());

            // Create output dir
            if (!std::filesystem::exists(output))
                std::filesystem::create_directories(output);

            pipeline.run(input, output, params, level);
        }
    }
} // namespace satdump
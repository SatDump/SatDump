#include "module.h"
#include "pipeline/module.h"

namespace satdump
{
    void ModuleCmdHandler::reg(CLI::App *app)
    {
        CLI::App *sub_module = app->add_subcommand("module", "Run a single module");
        for (auto &p : satdump::pipeline::modules_registry)
        {
            CLI::App *sub_p = sub_module->add_subcommand(p.id);
            sub_p->add_option("input_file");
            sub_p->add_option("output_hint");
            modules_opts.emplace(p.id, std::map<std::string, std::shared_ptr<std::string>>());
            for (auto &ep : p.params.items())
            {
                auto opt = std::make_shared<std::string>();
                auto f = sub_p->add_flag("--" + ep.key(), *opt, "");
                if (ep.value().is_string())
                    f->default_val(ep.value().get<std::string>());
                else
                    f->default_val(ep.value().dump());
                modules_opts[p.id].emplace(ep.key(), opt);
            }
        }
    }

    void ModuleCmdHandler::run(CLI::App *app, CLI::App *subcom)
    {
        for (auto *s2 : subcom->get_subcommands())
        {
            std::string mod_id = s2->get_name();
            std::string in = s2->get_option("input_file")->as<std::string>();
            std::string out = s2->get_option("output_hint")->as<std::string>();

            nlohmann::json params;

            for (auto *s33 : s2->get_options())
            {
                if (s2->count(s33->get_name()))
                {
                    // logger->critical(s33->get_name().substr(2));
                    auto optname = s33->get_name().substr(2);
                    if (modules_opts[mod_id].count(optname))
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

            auto inst = satdump::pipeline::getModuleInstance(mod_id, in, out, params);

            inst->setInputType(satdump::pipeline::DATA_FILE);
            inst->setOutputType(satdump::pipeline::DATA_FILE);

            inst->init();

            // TODOREWORK show stats
            inst->process();
        }
    }
} // namespace satdump
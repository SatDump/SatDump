#include "core/plugin.h"
#include "logger.h"

#include "core/config.h"

#include "loader/archive_loader.h"

#include "explorer/explorer.h"
#include "imgui/imgui_stdlib.h"

namespace
{
    bool _loader_open = false;
    std::unique_ptr<satdump::ArchiveLoader> _loader = nullptr;
} // namespace

void provideExplorerFileLoader(const satdump::explorer::ExplorerRequestFileLoad &evt);

class FirstPartyLoaderSupport : public satdump::Plugin
{
public:
    std::string getID() { return "first_party_loader_support"; }

    void init()
    {
        satdump::eventBus->register_handler<satdump::config::RegisterPluginConfigHandlersEvent>(registerConfigHandler);
        satdump::eventBus->register_handler<satdump::explorer::RenderLoadMenuElementsEvent>(renderExplorerLoaderButton);
        satdump::eventBus->register_handler<satdump::explorer::ExplorerRequestFileLoad>(provideExplorerFileLoader);
    }

    static void registerConfigHandler(const satdump::config::RegisterPluginConfigHandlersEvent &evt)
    {
        evt.plugin_config_handlers.push_back({"First-Party Support", FirstPartyLoaderSupport::renderConfig, FirstPartyLoaderSupport::saveConfig});
    }

public:
    static void initLoader()
    {
        if (!_loader)
        {
            _loader = std::make_unique<satdump::ArchiveLoader>();

            if (satdump::satdump_cfg.main_cfg.contains("plugin_settings") && satdump::satdump_cfg.main_cfg["plugin_settings"].contains("firstparty_products"))
            {
                auto &cfg = satdump::satdump_cfg.main_cfg["plugin_settings"]["firstparty_products"];
                if (cfg.contains("eumetsat_credentials_key"))
                    _loader->eumetsat_user_consumer_credential = cfg["eumetsat_credentials_key"];
                if (cfg.contains("eumetsat_credentials_secret"))
                    _loader->eumetsat_user_consumer_secret = cfg["eumetsat_credentials_secret"];
            }
        }
    }

    static void renderConfig()
    {
        initLoader();

        ImGui::Text("EUMETSAT User «Consumer Key»");
        ImGui::SameLine();
        ImGui::InputText("##eumetsattokenloader_key", &_loader->eumetsat_user_consumer_credential);

        ImGui::Text("EUMETSAT User «Consumer Secret»");
        ImGui::SameLine();
        ImGui::InputText("##eumetsattokenloader_secret", &_loader->eumetsat_user_consumer_secret);
    }

    static void saveConfig()
    {
        if (_loader == nullptr)
            return;
        satdump::satdump_cfg.main_cfg["plugin_settings"]["firstparty_products"] = {};
        auto &cfg = satdump::satdump_cfg.main_cfg["plugin_settings"]["firstparty_products"];
        cfg["eumetsat_credentials_key"] = _loader->eumetsat_user_consumer_credential;
        cfg["eumetsat_credentials_secret"] = _loader->eumetsat_user_consumer_secret;
    }

    static void renderExplorerLoaderButton(const satdump::explorer::RenderLoadMenuElementsEvent &evt)
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Load First-Party"))
                _loader_open = true;
            ImGui::EndMenu();
        }

        if (_loader_open)
        {
            initLoader();
            _loader->drawUI(&_loader_open);
        }
    }
};

PLUGIN_LOADER(FirstPartyLoaderSupport)

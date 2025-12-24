#include "core/plugin.h"
#include "init.h"
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

            std::string eumetsat_user_consumer_credential = satdump::db->get_user("firstparty_products_eumetsat_credentials_key", "");
            std::string eumetsat_user_consumer_secret = satdump::db->get_user("firstparty_products_eumetsat_credentials_secret", "");

            if (eumetsat_user_consumer_credential.size())
                _loader->eumetsat_user_consumer_credential = eumetsat_user_consumer_credential;
            if (eumetsat_user_consumer_secret.size())
                _loader->eumetsat_user_consumer_secret = eumetsat_user_consumer_secret;
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

        satdump::db->set_user("firstparty_products_eumetsat_credentials_key", _loader->eumetsat_user_consumer_credential);
        satdump::db->set_user("firstparty_products_eumetsat_credentials_secret", _loader->eumetsat_user_consumer_secret);
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

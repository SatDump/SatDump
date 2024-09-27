#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "nc2pro/module_nc2pro.h"

#include "nc2pro/fci_nc_calibrator.h"

//////////

#include "viewer/viewer.h"
#include "loader/archive_loader.h"
#include "imgui/imgui_stdlib.h"

namespace
{
    bool _enable_loader = false;
    bool _loader_open = false;
    std::unique_ptr<satdump::ArchiveLoader> _loader;
}

class OfficalProductsSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "official_products_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
        satdump::eventBus->register_handler<satdump::ImageProducts::RequestCalibratorEvent>(provideImageCalibratorHandler);

        satdump::eventBus->register_handler<satdump::config::RegisterPluginConfigHandlersEvent>(registerConfigHandler);
        satdump::eventBus->register_handler<satdump::ViewerApplication::RenderLoadMenuElementsEvent>(renderViewerLoaderButton);

        if (satdump::config::main_cfg.contains("plugin_settings") && satdump::config::main_cfg["plugin_settings"].contains("official_products"))
            if (satdump::config::main_cfg["plugin_settings"]["official_products"].contains("enable_loader"))
                _enable_loader = satdump::config::main_cfg["plugin_settings"]["official_products"]["enable_loader"];
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, nc2pro::Nc2ProModule);
    }

    static void provideImageCalibratorHandler(const satdump::ImageProducts::RequestCalibratorEvent &evt)
    {
        if (evt.id == "mtg_nc_fci")
            evt.calibrators.push_back(std::make_shared<nc2pro::FCINcCalibrator>(evt.calib, evt.products));
    }

    static void registerConfigHandler(const satdump::config::RegisterPluginConfigHandlersEvent &evt)
    {
        evt.plugin_config_handlers.push_back({"Official Products", OfficalProductsSupport::renderConfig, OfficalProductsSupport::saveConfig});
    }

public:
    static void initLoader()
    {
        if (!_loader)
        {
            _loader = std::make_unique<satdump::ArchiveLoader>();

            if (satdump::config::main_cfg.contains("plugin_settings") && satdump::config::main_cfg["plugin_settings"].contains("official_products"))
            {
                auto &cfg = satdump::config::main_cfg["plugin_settings"]["official_products"];
                if (cfg.contains("eumetsat_credentials_key"))
                    _loader->eumetsat_user_consumer_credential = cfg["eumetsat_credentials_key"];
                if (cfg.contains("eumetsat_credentials_secret"))
                    _loader->eumetsat_user_consumer_secret = cfg["eumetsat_credentials_secret"];
                if (cfg.contains("enable_loader"))
                    _enable_loader = cfg["enable_loader"];
            }
        }
    }

    static void renderConfig()
    {
        initLoader();

        ImGui::Checkbox("Enable Loader", &_enable_loader);

        ImGui::Text("EUMETSAT User Credentials Key");
        ImGui::SameLine();
        ImGui::InputText("##eumetsattokenloader_key", &_loader->eumetsat_user_consumer_credential);

        ImGui::Text("EUMETSAT User Credentials Secret");
        ImGui::SameLine();
        ImGui::InputText("##eumetsattokenloader_secret", &_loader->eumetsat_user_consumer_secret);
    }

    static void saveConfig()
    {
        satdump::config::main_cfg["plugin_settings"]["official_products"] = {};
        auto &cfg = satdump::config::main_cfg["plugin_settings"]["official_products"];
        cfg["eumetsat_credentials_key"] = _loader->eumetsat_user_consumer_credential;
        cfg["eumetsat_credentials_secret"] = _loader->eumetsat_user_consumer_secret;
        cfg["enable_loader"] = _enable_loader;
    }

    static void renderViewerLoaderButton(const satdump::ViewerApplication::RenderLoadMenuElementsEvent &evt)
    {
        if (!_enable_loader)
            return;

        if (ImGui::Button("Load Official"))
            _loader_open = true;

        if (_loader_open)
        {
            initLoader();
            _loader->drawUI(&_loader_open);
        }
    }
};

PLUGIN_LOADER(OfficalProductsSupport)
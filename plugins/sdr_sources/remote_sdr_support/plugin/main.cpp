#include "core/plugin.h"
#include "logger.h"
#include "remote_source.h"
#include "core/config.h"

#include "core/module.h"
#include "imgui/imgui.h"

std::vector<std::pair<std::string, int>> additional_servers;

class RemoteSDRSupport : public satdump::Plugin
{
public:
    static std::string address_to_add;
    static int port_to_add;

    static void renderConfig()
    {
        // ImGui::Text("This is the config!!");

        if (ImGui::BeginTable("##satdumpgeneralsettings", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            int i = 0;
            for (auto &servers : additional_servers)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::InputText(std::string("##ipinpuputremote" + std::to_string(i)).c_str(), &servers.first);
                ImGui::TableSetColumnIndex(1);
                ImGui::InputInt(std::string("##ipinpuputport" + std::to_string(i)).c_str(), &servers.second);
                i++;
            }
            ImGui::EndTable();
        }

        ImGui::SetNextItemWidth(200 * ui_scale);
        ImGui::InputText("##remoteaddaddress", &address_to_add);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100 * ui_scale);
        ImGui::InputInt("##remoteaddport", &port_to_add);
        ImGui::SameLine();
        if (ImGui::Button("Add###addremotenew"))
            additional_servers.push_back({address_to_add, port_to_add});
    }

    static void save()
    {
        logger->info("SAVED!");
    }

public:
    std::string getID()
    {
        return "remote_sdr_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
        satdump::eventBus->register_handler<satdump::config::RegisterPluginConfigHandlersEvent>(registerConfigHandler);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({RemoteSource::getID(), {RemoteSource::getInstance, RemoteSource::getAvailableSources}});
    }

    static void registerConfigHandler(const satdump::config::RegisterPluginConfigHandlersEvent &evt)
    {
        evt.plugin_config_handlers.push_back({"Remote SDR", RemoteSDRSupport::renderConfig, RemoteSDRSupport::save});
    }
};

std::string RemoteSDRSupport::address_to_add = "";
int RemoteSDRSupport::port_to_add = 0;

PLUGIN_LOADER(RemoteSDRSupport)
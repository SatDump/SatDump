#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

#include "core/plugin.h"
#include "logger.h"
#include "remote_source.h"
#include "core/config.h"
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

        if (additional_servers.size() > 0)
        {
            ImGuiStyle style = ImGui::GetStyle();
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(style.CellPadding.x, 5.0 * ui_scale));
            if (ImGui::BeginTable("##satdumpremotesdrsettings", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableSetupColumn("##satdumpremotesdrsettingsip");
                ImGui::TableSetupColumn("##satdumpremotesdrsettingsport");
                ImGui::TableSetupColumn("##satdumpremotesdrsettingsremove", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted("SDR Server");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted("Port");
                int i = 0;
                for (auto &servers : additional_servers)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::SetNextItemWidth(200 * ui_scale);
                    ImGui::InputTextWithHint(std::string("##ipinpuputremote" + std::to_string(i)).c_str(), "127.0.0.1", &servers.first, ImGuiInputTextFlags_CharsDecimal);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(100 * ui_scale);
                    if (ImGui::InputInt(std::string("##ipinpuputport" + std::to_string(i)).c_str(), &servers.second))
                    {
                        if (servers.second < 1)
                            servers.second = 1;
                        if (servers.second > 65535)
                            servers.second = 65535;
                    }
                    ImGui::TableSetColumnIndex(2);
                    if (ImGui::Button(std::string("Remove##remoteserverremove" + std::to_string(i)).c_str()))
                    {
                        additional_servers.erase(additional_servers.begin() + i);
                        break;
                    }
                    i++;
                }
                ImGui::EndTable();
            }
            ImGui::PopStyleVar();
        }

        ImGui::TextUnformatted("Add new Server: ");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200 * ui_scale);
        ImGui::InputTextWithHint("##remoteaddaddress", "127.0.0.1", &address_to_add, ImGuiInputTextFlags_CharsDecimal);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100 * ui_scale);
        if (ImGui::InputInt("##remoteaddport", &port_to_add))
        {
            if (port_to_add < 1)
                port_to_add = 1;
            if (port_to_add > 65535)
                port_to_add = 65535;
        }
        ImGui::SameLine();
        if (ImGui::Button("Add###addremotenew"))
        {
            struct sockaddr_in sa;
            if (inet_pton(AF_INET, address_to_add.c_str(), &(sa.sin_addr)) > 0)
            {
                additional_servers.push_back({address_to_add, port_to_add});
                address_to_add = "";
                port_to_add = 5656;
            }
            else
                logger->warn("Not adding invalid Remote SDR IP %s", address_to_add.c_str());
        }
    }

    static void save()
    {
        satdump::satdump_cfg.main_cfg["plugin_settings"]["remote_sdr_support"] = nlohmann::json::array();
        for (auto &server : additional_servers)
            satdump::satdump_cfg.main_cfg["plugin_settings"]["remote_sdr_support"].push_back({{"ip", server.first}, {"port", server.second}});
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
        for (auto &server : satdump::satdump_cfg.main_cfg["plugin_settings"]["remote_sdr_support"])
            additional_servers.push_back({server["ip"], server["port"]});
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
int RemoteSDRSupport::port_to_add = 5656;

PLUGIN_LOADER(RemoteSDRSupport)
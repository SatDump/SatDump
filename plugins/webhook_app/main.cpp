#include "core/config.h"
#include "core/plugin.h"
#include "logger.h"
#include "common/rimgui.h"
#include "nlohmann/json.hpp"

#include "webhook.h"

static std::vector<webhook_app::WebhookConfig> webhook_configs;

class Webhook : public satdump::Plugin
{
private:
    std::vector<std::unique_ptr<webhook_app::WebhookSender>> senders;

    static void renderConfig()
    {
        if (ImGui::Button("+ Add Webhook"))
            webhook_configs.push_back({});

        for (int i = 0; i < (int)webhook_configs.size(); i++)
        {
            ImGui::PushID(i);

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Webhook %d", i + 1);
            ImGui::SameLine();
            ImGui::BeginDisabled(!webhook_app::WebhookSender::is_valid_url(webhook_configs[i].url));
            if (ImGui::Button("Test"))
                webhook_app::WebhookSender::test(webhook_configs[i]);
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Remove"))
            {
                webhook_configs.erase(webhook_configs.begin() + i);
                ImGui::PopID();
                i--;
                continue;
            }

            ImGui::InputText("URL", &webhook_configs[i].url);
            ImGui::Checkbox("Pipeline Done", &webhook_configs[i].events.pipeline_done);
            ImGui::SameLine();
            ImGui::Checkbox("TLEs Updated", &webhook_configs[i].events.tles_updated);
            ImGui::SameLine();
            ImGui::Checkbox("Tracking AOS", &webhook_configs[i].events.tracking_aos);
            ImGui::SameLine();
            ImGui::Checkbox("Tracking LOS", &webhook_configs[i].events.tracking_los);

            ImGui::Separator();
            ImGui::PopID();
        }
    }

    static void save()
    {
        nlohmann::json webhooks = nlohmann::json::array();
        for (auto &cfg : webhook_configs)
        {
            if (!webhook_app::WebhookSender::is_valid_url(cfg.url))
                continue;
            webhooks.push_back({
                {"url", cfg.url},
                {"events", {
                    {"pipeline_done", cfg.events.pipeline_done},
                    {"tles_updated", cfg.events.tles_updated},
                    {"tracking_aos", cfg.events.tracking_aos},
                    {"tracking_los", cfg.events.tracking_los},
                }},
            });
        }
        satdump::satdump_cfg.main_cfg["plugin_settings"]["webhook_app"]["webhooks"] = webhooks;
    }

    static void registerConfigHandler(const satdump::config::RegisterPluginConfigHandlersEvent &evt)
    {
        evt.plugin_config_handlers.push_back({"Event Webhooks", Webhook::renderConfig, Webhook::save});
    }

public:
    std::string getID() { return "webhook_app"; }

    void init()
    {
        satdump::eventBus->register_handler<satdump::config::RegisterPluginConfigHandlersEvent>(registerConfigHandler);

        auto &cfg = satdump::satdump_cfg.main_cfg["plugin_settings"]["webhook_app"];
        if (cfg.contains("webhooks") && cfg["webhooks"].is_array())
        {
            for (auto &entry : cfg["webhooks"])
            {
                webhook_app::WebhookConfig wc;
                if (entry.contains("url") && entry["url"].is_string())
                    wc.url = entry["url"].get<std::string>();
                if (entry.contains("events"))
                {
                    auto &ev = entry["events"];
                    if (ev.contains("pipeline_done") && ev["pipeline_done"].is_boolean())
                        wc.events.pipeline_done = ev["pipeline_done"].get<bool>();
                    if (ev.contains("tles_updated") && ev["tles_updated"].is_boolean())
                        wc.events.tles_updated = ev["tles_updated"].get<bool>();
                    if (ev.contains("tracking_aos") && ev["tracking_aos"].is_boolean())
                        wc.events.tracking_aos = ev["tracking_aos"].get<bool>();
                    if (ev.contains("tracking_los") && ev["tracking_los"].is_boolean())
                        wc.events.tracking_los = ev["tracking_los"].get<bool>();
                }
                webhook_configs.push_back(wc);
            }
        }

        for (auto &hook : webhook_configs)
        {
            if (webhook_app::WebhookSender::is_valid_url(hook.url))
                senders.push_back(std::make_unique<webhook_app::WebhookSender>(hook));
        }

        logger->info("Webhook plugin!");
    }
};

PLUGIN_LOADER(Webhook)

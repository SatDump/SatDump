#include "core/config.h"
#include "core/plugin.h"
#include "logger.h"
#include "utils/http.h"
#include "init.h"
#include "nlohmann/json.hpp"
#include "utils/time.h"

#include "common/rimgui.h"                          // renderConfig
#include "pipeline/pipeline.h"                      // PipelineDoneProcessingEvent
#include "common/tracking/scheduler/scheduler.h"    // TrackingSchedulerAOSEvent, TrackingSchedulerLOSEvent

std::string webhook_url = "";

class Webhook : public satdump::Plugin
{
private:
    static void renderConfig() { ImGui::InputText("Webhook URL", &webhook_url); }

    static void save() { satdump::satdump_cfg.main_cfg["plugin_settings"]["webhook"]["url"] = webhook_url; }

    void send_webhook(std::string message)
    {
        nlohmann::json payload;
        if (webhook_url.find("discordapp.com/api/webhooks/") != std::string::npos)
            payload["content"] = message;
        else if (webhook_url.find("hooks.slack.com/services/") != std::string::npos)
            payload["text"] = message;
        else {
            payload["message"] = message;
        }

        std::string json = payload.dump();
        logger->debug("Sending webhook: \"" + json + "\"");

        std::string response;
        satdump::perform_http_request_post(webhook_url, response, json, "Content-Type: application/json");
    }

    void handle_event(const satdump::SatDumpStartedEvent &evt)
    {
        send_webhook("Started SatDump");
    }

    void handle_event(const satdump::pipeline::events::PipelineDoneProcessingEvent &evt)
    {
        satdump::pipeline::Pipeline pipeline = satdump::pipeline::getPipelineFromID(evt.pipeline_id);
        send_webhook("Finished processing **" + pipeline.name + "** pipeline");
    }

    void handle_event(const satdump::events::TrackingSchedulerAOSEvent &evt)
    {
        std::string object_name = satdump::db_tle->get_from_norad(evt.pass.norad)->name;

        std::stringstream elevation;
        elevation << std::fixed << std::setprecision(2) << evt.pass.max_elevation;

        std::string timestamp = satdump::timestamp_to_string(evt.pass.los_time, true);
        std::string::size_type timestamp_delim = timestamp.find(" ");
        std::string los_time = timestamp.substr(timestamp_delim + 1);

        std::stringstream message;
        message << "AOS **" << object_name << "**: Tracking ";
        message << elevation.str() << "° pass until " << los_time;
        send_webhook(message.str());
    }

    void handle_event(const satdump::events::TrackingSchedulerLOSEvent &evt)
    {
        std::string object_name = satdump::db_tle->get_from_norad(evt.pass.norad)->name;
        send_webhook("LOS **" + object_name + "**");
    }

public:
    std::string getID() { return "webhook"; }

    void init()
    {
        satdump::eventBus->register_handler<satdump::config::RegisterPluginConfigHandlersEvent>(registerConfigHandler);
        if (!satdump::satdump_cfg.main_cfg["plugin_settings"]["webhook"]["url"].is_null())
            webhook_url = satdump::satdump_cfg.main_cfg["plugin_settings"]["webhook"]["url"];

        if (webhook_url != "")
#ifdef BUILD_IS_DEBUG
            satdump::eventBus->register_handler<satdump::SatDumpStartedEvent>(
                [this](const satdump::SatDumpStartedEvent &evt){ this->handle_event(evt); }
            );
#endif
            satdump::eventBus->register_handler<satdump::pipeline::events::PipelineDoneProcessingEvent>(
                [this](const satdump::pipeline::events::PipelineDoneProcessingEvent &evt){ this->handle_event(evt); }
            );
            satdump::eventBus->register_handler<satdump::events::TrackingSchedulerAOSEvent>(
                [this](const satdump::events::TrackingSchedulerAOSEvent &evt){ this->handle_event(evt); }
            );
            satdump::eventBus->register_handler<satdump::events::TrackingSchedulerLOSEvent>(
                [this](const satdump::events::TrackingSchedulerLOSEvent &evt){ this->handle_event(evt); }
            );

        logger->info("Webhook plugin!");
    }

    static void registerConfigHandler(const satdump::config::RegisterPluginConfigHandlersEvent &evt)
    {
        evt.plugin_config_handlers.push_back({"Event Webhooks", Webhook::renderConfig, Webhook::save});
    }
};

PLUGIN_LOADER(Webhook)

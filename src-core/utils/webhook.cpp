#include "webhook.h"
#include "core/config.h"
#include "http.h"
#include "init.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "utils/time.h"

namespace satdump
{
    Webhook::Webhook()
    {
#ifdef BUILD_IS_DEBUG
        eventBus->register_handler<SatDumpStartedEvent>(
            [this](const SatDumpStartedEvent &evt){ this->handle_event(evt); }
        );
#endif
        eventBus->register_handler<pipeline::events::PipelineDoneProcessingEvent>(
            [this](const pipeline::events::PipelineDoneProcessingEvent &evt){ this->handle_event(evt); }
        );
        eventBus->register_handler<events::TrackingSchedulerAOSEvent>(
            [this](const events::TrackingSchedulerAOSEvent &evt){ this->handle_event(evt); }
        );
        eventBus->register_handler<events::TrackingSchedulerLOSEvent>(
            [this](const events::TrackingSchedulerLOSEvent &evt){ this->handle_event(evt); }
        );
    }

    Webhook::~Webhook(){}

    void Webhook::send_webhook(std::string message)
    {
        std::string url = satdump_cfg.getValueFromSatDumpGeneral<std::string>("webhook_url");

        nlohmann::json payload;
        if (url.find("discord.com/api/webhooks/") != std::string::npos)
            payload["content"] = message;
        else if (url.find("hooks.slack.com/services/") != std::string::npos)
            payload["text"] = message;
        else {
            payload["message"] = message;
        }

        std::string json = payload.dump();
        logger->debug("Sending webhook: \"" + json + "\"");

        std::string response;
        satdump::perform_http_request_post(url, response, json, "Content-Type: application/json");
    }

    void Webhook::handle_event(const SatDumpStartedEvent &evt)
    {
        send_webhook("Started SatDump");
    }

    void Webhook::handle_event(const pipeline::events::PipelineDoneProcessingEvent &evt)
    {
        pipeline::Pipeline pipeline = pipeline::getPipelineFromID(evt.pipeline_id);
        send_webhook("Finished processing **" + pipeline.name + "** pipeline");
    }

    void Webhook::handle_event(const events::TrackingSchedulerAOSEvent &evt)
    {
        std::string object_name = db_tle->get_from_norad(evt.pass.norad)->name;

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

    void Webhook::handle_event(const events::TrackingSchedulerLOSEvent &evt)
    {
        std::string object_name = db_tle->get_from_norad(evt.pass.norad)->name;
        send_webhook("LOS **" + object_name + "**");
    }
} // namespace satdump
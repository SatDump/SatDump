#include "webhook.h"

#include <regex>
#include <sstream>
#include <iomanip>

#include "init.h"
#include "logger.h"
#include "utils/http.h"
#include "utils/time.h"
#include "nlohmann/json.hpp"

namespace webhook_app
{
    bool WebhookSender::is_valid_url(const std::string &url)
    {
        static const std::regex url_regex(R"(^https?://\S+$)", std::regex::icase);
        return std::regex_match(url, url_regex);
    }

    std::string WebhookSender::get_payload_key(const std::string &url)
    {
        static const std::pair<std::string_view, std::string_view> platform_keys[] = {
            {"discord.com/api/webhooks/", "content"},
            {"discordapp.com/api/webhooks/", "content"},
            {"hooks.slack.com/services/",    "text"},
        };

        for (auto &[substr, key] : platform_keys)
            if (url.find(substr) != std::string::npos)
                return std::string(key);

        return "message";
    }

    WebhookSender::WebhookSender(const WebhookConfig &config)
        : url(config.url), payload_key(get_payload_key(config.url)), events(config.events)
    {

#ifdef BUILD_IS_DEBUG
        satdump::eventBus->register_handler<satdump::SatDumpStartedEvent>(
            [this](const satdump::SatDumpStartedEvent &evt) { handle_event(evt); });
#endif

        if (events.pipeline_done)
            satdump::eventBus->register_handler<satdump::pipeline::events::PipelineDoneProcessingEvent>(
                [this](const satdump::pipeline::events::PipelineDoneProcessingEvent &evt) { handle_event(evt); });

        if (events.tles_updated)
            satdump::eventBus->register_handler<satdump::TLEsUpdatedEvent>(
                [this](const satdump::TLEsUpdatedEvent &evt) { handle_event(evt); });

        if (events.tracking_aos)
            satdump::eventBus->register_handler<satdump::events::TrackingSchedulerAOSEvent>(
                [this](const satdump::events::TrackingSchedulerAOSEvent &evt) { handle_event(evt); });

        if (events.tracking_los)
            satdump::eventBus->register_handler<satdump::events::TrackingSchedulerLOSEvent>(
                [this](const satdump::events::TrackingSchedulerLOSEvent &evt) { handle_event(evt); });
    }

    void WebhookSender::send_webhook(const std::string &url, const std::string &payload_key, const std::string &message)
    {
        nlohmann::json payload;
        payload[payload_key] = message;

        std::string json = payload.dump();
        logger->debug("Sending webhook: \"" + json + "\"");

        std::string response;
        satdump::perform_http_request_post(url, response, json, "Content-Type: application/json");
    }

    void WebhookSender::test(const WebhookConfig &config)
    {
        send_webhook(config.url, get_payload_key(config.url), "Hello from SatDump!");
    }

    void WebhookSender::handle_event(const satdump::SatDumpStartedEvent &)
    {
        send_webhook(url, payload_key, "Started SatDump");
    }

    void WebhookSender::handle_event(const satdump::TLEsUpdatedEvent &)
    {
        send_webhook(url, payload_key, "TLEs Updated");
    }

    void WebhookSender::handle_event(const satdump::pipeline::events::PipelineDoneProcessingEvent &evt)
    {
        satdump::pipeline::Pipeline pipeline = satdump::pipeline::getPipelineFromID(evt.pipeline_id);
        send_webhook(url, payload_key, "Finished processing **" + pipeline.name + "** pipeline");
    }

    void WebhookSender::handle_event(const satdump::events::TrackingSchedulerAOSEvent &evt)
    {
        std::string object_name = satdump::db_tle->get_from_norad(evt.pass.norad)->name;

        std::stringstream elevation;
        elevation << std::fixed << std::setprecision(2) << evt.pass.max_elevation;

        std::string timestamp = satdump::timestamp_to_string(evt.pass.los_time, true);
        auto timestamp_delim = timestamp.find(" ");
        std::string los_time = timestamp.substr(timestamp_delim + 1);

        std::stringstream message;
        message << "AOS **" << object_name << "**: Tracking " << elevation.str() << "° pass until " << los_time;
        send_webhook(url, payload_key, message.str());
    }

    void WebhookSender::handle_event(const satdump::events::TrackingSchedulerLOSEvent &evt)
    {
        std::string object_name = satdump::db_tle->get_from_norad(evt.pass.norad)->name;
        send_webhook(url, payload_key, "LOS **" + object_name + "**");
    }
}

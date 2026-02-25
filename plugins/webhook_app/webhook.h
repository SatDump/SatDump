#pragma once

#include <string>

#include "core/plugin.h"
#include "common/tracking/tle.h"
#include "pipeline/pipeline.h"
#include "common/tracking/scheduler/scheduler.h"

namespace webhook_app
{
    struct WebhookEvents
    {
        bool pipeline_done = true;
        bool tles_updated = true;
        bool tracking_aos = true;
        bool tracking_los = true;
    };

    struct WebhookConfig
    {
        std::string url;
        WebhookEvents events;
    };

    class WebhookSender
    {
    private:
        std::string url;
        std::string payload_key;
        WebhookEvents events;

        static std::string get_payload_key(const std::string &url);
        static void send_webhook(const std::string &url, const std::string &payload_key, const std::string &message);

        void handle_event(const satdump::SatDumpStartedEvent &evt);
        void handle_event(const satdump::pipeline::events::PipelineDoneProcessingEvent &evt);
        void handle_event(const satdump::TLEsUpdatedEvent &evt);
        void handle_event(const satdump::events::TrackingSchedulerAOSEvent &evt);
        void handle_event(const satdump::events::TrackingSchedulerLOSEvent &evt);

    public:
        WebhookSender(const WebhookConfig &config);

        static bool is_valid_url(const std::string &url);
        static void test(const WebhookConfig &config);
    };
}

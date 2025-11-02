#pragma once

/**
 * @file webhook.h
 * @brief Webhooks triggered by the event bus
 */

#include <string>
#include <iostream>

#include "core/plugin.h"                            // SatDumpStartedEvent
#include "pipeline/pipeline.h"                      // PipelineDoneProcessingEvent
#include "common/tracking/scheduler/scheduler.h"    // TrackingSchedulerAOSEvent, TrackingSchedulerLOSEvent

namespace satdump
{
    /**
     * @brief 
     */
    class Webhook
    {
    private:
        void send_webhook(std::string message);
        void handle_event(const SatDumpStartedEvent &evt);
        void handle_event(const pipeline::events::PipelineDoneProcessingEvent &evt);
        void handle_event(const events::TrackingSchedulerAOSEvent &evt);
        void handle_event(const events::TrackingSchedulerLOSEvent &evt);

    public:
        Webhook();
        ~Webhook();
    };
}
#pragma once

#include "common/ccsds/ccsds.h"
#include "common/tracking/tracking.h"
#include "utils/event_bus.h"
#include <memory>
#include <thread>

namespace satdump
{
    namespace ominous
    {
        struct PushSpacePacketEvent
        {
            int vcid;
            ccsds::CCSDSPacket pkt;
        };

        struct WorkerCore
        {
            std::shared_ptr<SatelliteTracker> tracker;
            std::shared_ptr<EventBus> pkt_bus;

            void pushPkt(int vcid, ccsds::CCSDSPacket pkt) { pkt_bus->fire_event<PushSpacePacketEvent>({vcid, pkt}); }
        };

        class InstrumentWorkerBase
        {
        private:
            bool thread_should_run;
            std::thread background_thread;

            void thread_func()
            {
                while (thread_should_run)
                    loop();
            }

        protected:
            WorkerCore core;

        public:
            InstrumentWorkerBase(WorkerCore core) : core(core) {}

            void start_th()
            {
                thread_should_run = true;
                background_thread = std::thread(&InstrumentWorkerBase::thread_func, this);
            }

            ~InstrumentWorkerBase()
            {
                thread_should_run = false;
                if (background_thread.joinable())
                    background_thread.join();
            }

            virtual void loop() = 0;
        };
    } // namespace ominous
} // namespace satdump
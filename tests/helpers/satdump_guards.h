#pragma once

/*
Save / restore guards for SatDump global state, shared by the whole suite.
Everything is inline and lives in the test_helpers namespace, as every test
file is linked into the same executable. Kept apart from helpers/guards.h as
these pull in SatDump headers. For a plain global variable (LIBPATH,
RESPATH, user_path...) use test_helpers::ValueGuard from helpers/guards.h.

- ConfigGuard saves satdump::satdump_cfg.main_cfg and restores it on
  destruction. Code reading the config with operator[] inserts null keys,
  so even "read-only" use has to be undone.
- EventBusGuard swaps in a fresh satdump::eventBus, so handlers registered
  by a test (which cannot be unregistered) disappear afterwards.
*/

#include "core/config.h"
#include "core/plugin.h"
#include <memory>

namespace test_helpers
{
    // Saves the global SatDump config and restores it on destruction
    class ConfigGuard
    {
    public:
        ConfigGuard() : saved(satdump::satdump_cfg.main_cfg) {}
        ~ConfigGuard() { satdump::satdump_cfg.main_cfg = saved; }

        ConfigGuard(const ConfigGuard &) = delete;
        ConfigGuard &operator=(const ConfigGuard &) = delete;

    private:
        nlohmann::ordered_json saved;
    };

    // Replaces the global event bus with a fresh one, restores the old one on destruction
    class EventBusGuard
    {
    public:
        EventBusGuard() : saved(satdump::eventBus) { satdump::eventBus = std::make_shared<satdump::EventBus>(); }
        ~EventBusGuard() { satdump::eventBus = saved; }

        EventBusGuard(const EventBusGuard &) = delete;
        EventBusGuard &operator=(const EventBusGuard &) = delete;

    private:
        std::shared_ptr<satdump::EventBus> saved;
    };
} // namespace test_helpers

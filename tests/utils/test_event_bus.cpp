#include "utils/event_bus.h"
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <thread>
#include <typeinfo>
#include <vector>

namespace
{
    struct PingEvent
    {
        int value;
    };

    struct PongEvent
    {
        std::string text;
    };

    // Events can carry references so handlers can return data to the caller
    struct CollectEvent
    {
        std::vector<std::string> &out;
    };
} // namespace

TEST_CASE("EventBus fires handlers registered for the event type", "[utils][event_bus]")
{
    satdump::EventBus bus;
    std::vector<int> got;
    bus.register_handler<PingEvent>([&](PingEvent e) { got.push_back(e.value); });

    bus.fire_event<PingEvent>({7});
    bus.fire_event<PingEvent>({-3});
    CHECK(got == std::vector<int>{7, -3});
}

TEST_CASE("EventBus ignores handlers of other event types", "[utils][event_bus]")
{
    satdump::EventBus bus;
    int pings = 0, pongs = 0;
    std::string last;
    bus.register_handler<PingEvent>([&](PingEvent) { pings++; });
    bus.register_handler<PongEvent>([&](PongEvent e) { pongs++; last = e.text; });

    bus.fire_event<PongEvent>({"hello"});
    CHECK(pings == 0);
    CHECK(pongs == 1);
    CHECK(last == "hello");

    bus.fire_event<PingEvent>({1});
    CHECK(pings == 1);
    CHECK(pongs == 1);
}

TEST_CASE("EventBus firing with no handlers is a no-op", "[utils][event_bus]")
{
    satdump::EventBus bus;
    CHECK_NOTHROW(bus.fire_event<PingEvent>({1}));
    PingEvent e{2};
    CHECK_NOTHROW(bus.fire_event(&e, typeid(PingEvent).name()));
}

TEST_CASE("EventBus calls every handler in registration order", "[utils][event_bus]")
{
    satdump::EventBus bus;
    std::vector<std::string> order;
    bus.register_handler<PingEvent>([&](PingEvent) { order.push_back("first"); });
    bus.register_handler<PongEvent>([&](PongEvent) { order.push_back("pong"); });
    bus.register_handler<PingEvent>([&](PingEvent) { order.push_back("second"); });
    bus.register_handler<PingEvent>([&](PingEvent) { order.push_back("third"); });

    bus.fire_event<PingEvent>({0});
    CHECK(order == std::vector<std::string>{"first", "second", "third"});
}

TEST_CASE("EventBus handlers receive a copy of the event", "[utils][event_bus]")
{
    satdump::EventBus bus;
    int modified = 0;
    bus.register_handler<PingEvent>(
        [&](PingEvent e)
        {
            e.value = 1000;
            modified = e.value;
        });
    int seen = 0;
    bus.register_handler<PingEvent>([&](PingEvent e) { seen = e.value; });

    PingEvent evt{5};
    bus.fire_event(evt);
    CHECK(modified == 1000);
    CHECK(evt.value == 5);
    CHECK(seen == 5);
}

TEST_CASE("EventBus events holding references let handlers write back", "[utils][event_bus]")
{
    satdump::EventBus bus;
    bus.register_handler<CollectEvent>([](CollectEvent e) { e.out.push_back("a"); });
    bus.register_handler<CollectEvent>([](CollectEvent e) { e.out.push_back("b"); });

    std::vector<std::string> results;
    bus.fire_event<CollectEvent>({results});
    CHECK(results == std::vector<std::string>{"a", "b"});
}

TEST_CASE("EventBus fire_event by name dispatches to the matching type", "[utils][event_bus]")
{
    satdump::EventBus bus;
    int got = 0;
    bool pong = false;
    bus.register_handler<PingEvent>([&](PingEvent e) { got = e.value; });
    bus.register_handler<PongEvent>([&](PongEvent) { pong = true; });

    PingEvent e{42};
    bus.fire_event(&e, typeid(PingEvent).name());
    CHECK(got == 42);
    CHECK_FALSE(pong);

    bus.fire_event(&e, "not a registered type name");
    CHECK(got == 42);
    CHECK_FALSE(pong);
}

TEST_CASE("EventBus handlers registered while firing run on the next fire", "[utils][event_bus]")
{
    satdump::EventBus bus;
    int inner_calls = 0;
    bool registered = false;
    bus.register_handler<PingEvent>(
        [&](PingEvent)
        {
            if (!registered)
            {
                registered = true;
                bus.register_handler<PingEvent>([&](PingEvent) { inner_calls++; });
            }
        });

    // The handler list is copied before dispatch, so the new handler is not called yet
    bus.fire_event<PingEvent>({0});
    CHECK(inner_calls == 0);
    bus.fire_event<PingEvent>({0});
    CHECK(inner_calls == 1);
}

TEST_CASE("EventBus supports concurrent firing and registration", "[utils][event_bus]")
{
    satdump::EventBus bus;
    std::atomic<int> calls{0};
    bus.register_handler<PingEvent>([&](PingEvent) { calls++; });

    std::vector<std::thread> threads;
    for (int t = 0; t < 4; t++)
        threads.emplace_back(
            [&]()
            {
                for (int i = 0; i < 250; i++)
                    bus.fire_event<PingEvent>({i});
            });
    std::thread registrar(
        [&]()
        {
            for (int i = 0; i < 50; i++)
                bus.register_handler<PongEvent>([](PongEvent) {});
        });
    for (auto &t : threads)
        t.join();
    registrar.join();

    CHECK(calls == 1000);
}

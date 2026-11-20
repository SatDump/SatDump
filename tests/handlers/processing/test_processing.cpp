#include "handlers/processing/processing.h"
#include "../../pipeline/pipeline_test_helpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace pipeline_test_helpers;
namespace pl = satdump::pipeline;
using satdump::handlers::OffProcessingHandler;

TEST_CASE("OffProcessingHandler with an unknown pipeline ID does not start processing", "[handlers][processing]")
{
    PipelinesStateGuard state;
    CallsGuard calls;

    {
        OffProcessingHandler h("unit_no_such_pipeline", "baseband", "/nonexistent/in.raw", "/nonexistent/out", {});
        CHECK(h.getName() == "Pipeline");
        CHECK(h.getID() == "processing_handler");
    }
    CHECK(callsOf("construct").empty());
}

TEST_CASE("OffProcessingHandler runs the pipeline in the background", "[handlers][processing]")
{
    ModuleRegistryGuard reg;
    EventBusGuard bus;
    MainConfigGuard cfg;
    CallsGuard calls;
    TempDir tmp("handlers_offproc");

    satdump::satdump_cfg.main_cfg["user_interface"]["open_explorer_post_processing"]["value"] = false;
    pl::modules_registry.push_back(recordingEntry("unit_off_a"));

    auto done = std::make_shared<std::atomic<int>>(0);
    satdump::eventBus->register_handler<pl::events::PipelineDoneProcessingEvent>([done](pl::events::PipelineDoneProcessingEvent) { (*done)++; });

    pl::Pipeline p;
    p.id = "unit_off";
    p.name = "Unit Off";
    p.live = false;
    p.steps = {{"baseband", "", {}, ""}, {"frames", "unit_off_a", {}, ""}};

    std::string input = tmp.file("in.raw");
    writeFile(input, "IN");
    std::string out = tmp.file("new/out/dir"); // created by the handler

    {
        OffProcessingHandler h(p, "baseband", input, out, {{"user", 1}});
        // Destruction waits for the processing thread
    }

    CHECK(std::filesystem::is_directory(out));
    CHECK(*done == 1);
    auto proc = callsOf("process");
    REQUIRE(proc.size() == 1);
    CHECK(proc[0].params["user"] == 1);
    CHECK(readFile(out + "/unit_off_unit_off_a.bin") == "IN[unit_off_a]");
}

TEST_CASE("OffProcessingHandler catches pipeline errors", "[handlers][processing]")
{
    ModuleRegistryGuard reg;
    EventBusGuard bus;
    MainConfigGuard cfg;
    TempDir tmp("handlers_offproc");

    pl::Pipeline p;
    p.id = "unit_off_err";
    p.name = "Unit Off Err";
    p.live = false;
    p.steps = {{"baseband", "", {}, ""}, {"frames", "unit_off_unregistered", {}, ""}};

    std::string input = tmp.file("in.raw");
    writeFile(input, "IN");

    auto done = std::make_shared<std::atomic<int>>(0);
    satdump::eventBus->register_handler<pl::events::PipelineDoneProcessingEvent>([done](pl::events::PipelineDoneProcessingEvent) { (*done)++; });

    {
        // The run throws inside the worker thread; the handler must swallow it
        OffProcessingHandler h(p, "baseband", input, tmp.file("out"), {});
    }
    CHECK(*done == 0);
}

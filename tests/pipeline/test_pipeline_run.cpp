#include "core/exception.h"
#include "pipeline/pipeline.h"
#include "pipeline_test_helpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace pipeline_test_helpers;
namespace pl = satdump::pipeline;

namespace
{
    // A test environment with a fresh registry, event bus, call log, and output dir
    struct RunEnv
    {
        ModuleRegistryGuard reg;
        EventBusGuard bus;
        CallsGuard calls;
        MainConfigGuard cfg;
        TempDir dir;
        std::string input;
        std::string out;

        RunEnv() : dir("pipe_run")
        {
            input = dir.file("input.raw");
            out = dir.file("out");
            std::filesystem::create_directories(out);
            writeFile(input, "DATA");
        }
    };

    pl::Pipeline makePipeline(const std::string &id, std::vector<pl::Pipeline::PipelineStep> steps)
    {
        pl::Pipeline p;
        p.id = id;
        p.name = id;
        p.live = false;
        p.steps = steps;
        return p;
    }

    std::vector<std::string> processedIds()
    {
        std::vector<std::string> ids;
        for (auto &c : callsOf("process"))
            ids.push_back(c.id);
        return ids;
    }
} // namespace

TEST_CASE("Pipeline::run chains module outputs from the step after the input level", "[pipeline][pipeline_run]")
{
    RunEnv env;
    pl::modules_registry.push_back(recordingEntry("unit_a"));
    pl::modules_registry.push_back(recordingEntry("unit_b"));
    pl::modules_registry.push_back(recordingEntry("unit_c"));

    auto p = makePipeline("unit_chain", {{"baseband", "", {}, ""}, {"soft", "unit_a", {}, ""}, {"frames", "unit_b", {}, ""}, {"products", "unit_c", {}, ""}});

    std::string final_file;
    p.run(env.input, env.out, {}, "soft", false, nullptr, nullptr, &final_file);

    // Data was already at "soft", so only frames and products are produced
    CHECK(processedIds() == std::vector<std::string>{"unit_b", "unit_c"});

    auto proc = callsOf("process");
    REQUIRE(proc.size() == 2);
    CHECK(proc[0].input == env.input);
    CHECK(proc[0].hint == env.out + "/unit_chain");
    CHECK(proc[0].input_type == pl::DATA_FILE);
    CHECK(proc[0].output_type == pl::DATA_FILE);
    CHECK(proc[1].input == env.out + "/unit_chain_unit_b.bin");
    CHECK(proc[1].hint == env.out + "/unit_chain");

    CHECK(final_file == env.out + "/unit_chain_unit_c.bin");
    CHECK(readFile(final_file) == "DATA[unit_b][unit_c]");
}

TEST_CASE("Pipeline::run from baseband runs file-only modules sequentially", "[pipeline][pipeline_run]")
{
    RunEnv env;
    pl::modules_registry.push_back(recordingEntry("unit_a"));
    pl::modules_registry.push_back(recordingEntry("unit_b"));
    pl::modules_registry.push_back(recordingEntry("unit_c"));

    auto p = makePipeline("unit_seq", {{"baseband", "", {}, ""}, {"soft", "unit_a", {}, ""}, {"frames", "unit_b", {}, ""}, {"products", "unit_c", {}, ""}});

    std::string final_file;
    p.run(env.input, env.out, {}, "baseband", false, nullptr, nullptr, &final_file);

    CHECK(processedIds() == std::vector<std::string>{"unit_a", "unit_b", "unit_c"});
    for (auto &c : callsOf("process"))
    {
        CHECK(c.input_type == pl::DATA_FILE);
        CHECK(c.output_type == pl::DATA_FILE);
    }
    CHECK(readFile(final_file) == "DATA[unit_a][unit_b][unit_c]");
}

TEST_CASE("Pipeline::run with disable_multi_modules does not probe the first two modules", "[pipeline][pipeline_run]")
{
    RunEnv env;
    pl::modules_registry.push_back(recordingEntry("unit_a", true, true));
    pl::modules_registry.push_back(recordingEntry("unit_b", true, true));
    pl::modules_registry.push_back(recordingEntry("unit_c"));

    auto p = makePipeline("unit_nomulti", {{"baseband", "", {}, ""}, {"soft", "unit_a", {}, ""}, {"frames", "unit_b", {}, ""}, {"products", "unit_c", {}, ""}});

    std::string final_file;
    p.run(env.input, env.out, {{"disable_multi_modules", true}}, "baseband", false, nullptr, nullptr, &final_file);

    // One instance per step, each running from a file, even though both could stream
    CHECK(callsOf("construct").size() == 3);
    CHECK(processedIds() == std::vector<std::string>{"unit_a", "unit_b", "unit_c"});
    for (auto &c : callsOf("process"))
        CHECK(c.input_type == pl::DATA_FILE);
    CHECK(readFile(final_file) == "DATA[unit_a][unit_b][unit_c]");
}

TEST_CASE("Pipeline::run streams between the first two modules when both support it", "[pipeline][pipeline_run]")
{
    RunEnv env;
    pl::modules_registry.push_back(recordingEntry("unit_a", false, true));
    pl::modules_registry.push_back(recordingEntry("unit_b", true, false));
    pl::modules_registry.push_back(recordingEntry("unit_c"));

    auto p = makePipeline("unit_stream", {{"baseband", "", {}, ""}, {"soft", "unit_a", {}, ""}, {"frames", "unit_b", {}, ""}, {"products", "unit_c", {}, ""}});

    std::string final_file;
    p.run(env.input, env.out, {}, "baseband", false, nullptr, nullptr, &final_file);

    auto proc = callsOf("process");
    REQUIRE(proc.size() == 3);

    // The first two run concurrently, so their order in the log is not fixed
    const ModuleCall *a = nullptr, *b = nullptr;
    for (auto &c : proc)
    {
        if (c.id == "unit_a")
            a = &c;
        if (c.id == "unit_b")
            b = &c;
    }
    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    CHECK(a->input == env.input);
    CHECK(a->input_type == pl::DATA_FILE);
    CHECK(a->output_type == pl::DATA_STREAM);
    CHECK(b->input_type == pl::DATA_STREAM);
    CHECK(b->output_type == pl::DATA_FILE);

    CHECK(proc[2].id == "unit_c");
    CHECK(proc[2].input == env.out + "/unit_stream_unit_b.bin");

    CHECK(readFile(env.out + "/unit_stream_unit_b.bin") == "DATA[unit_a][unit_b]");
    CHECK(readFile(final_file) == "DATA[unit_a][unit_b][unit_c]");
}

TEST_CASE("Pipeline::run passes merged parameters and honours input_override", "[pipeline][pipeline_run]")
{
    RunEnv env;
    pl::modules_registry.push_back(recordingEntry("unit_a"));
    pl::modules_registry.push_back(recordingEntry("unit_b"));

    writeFile(env.out + "/side_input.bin", "SIDE");

    auto p = makePipeline("unit_params", {{"baseband", "", {}, ""},
                                          {"soft", "unit_a", {{"symbolrate", 1000}, {"alpha", 0.5}}, ""},
                                          {"frames", "unit_b", {{"input_override", "side_input.bin"}}, "side_input.bin"}});

    std::string final_file;
    p.run(env.input, env.out, {{"alpha", 0.25}, {"user", "x"}, {"disable_multi_modules", true}}, "baseband", false, nullptr, nullptr, &final_file);

    auto proc = callsOf("process");
    REQUIRE(proc.size() == 2);
    CHECK(proc[0].params["symbolrate"] == 1000);
    CHECK(proc[0].params["alpha"] == 0.25);
    CHECK(proc[0].params["user"] == "x");
    CHECK(proc[1].params["user"] == "x");

    // The second step reads the override file from the output directory, not the previous output
    CHECK(proc[1].input == env.out + "/side_input.bin");
    CHECK(readFile(final_file) == "SIDE[unit_b]");
}

TEST_CASE("Pipeline::run does nothing when the input file does not exist", "[pipeline][pipeline_run]")
{
    RunEnv env;
    pl::modules_registry.push_back(recordingEntry("unit_a"));

    auto p = makePipeline("unit_noinput", {{"baseband", "", {}, ""}, {"soft", "unit_a", {}, ""}});

    bool event_fired = false;
    satdump::eventBus->register_handler<pl::events::PipelineDoneProcessingEvent>([&event_fired](pl::events::PipelineDoneProcessingEvent) { event_fired = true; });

    std::string final_file = "untouched";
    p.run(env.dir.file("does_not_exist.raw"), env.out, {}, "baseband", false, nullptr, nullptr, &final_file);

    CHECK(callsOf("construct").empty());
    CHECK(final_file == "untouched");
    CHECK_FALSE(event_fired);
}

TEST_CASE("Pipeline::run with an unknown input level runs nothing", "[pipeline][pipeline_run]")
{
    RunEnv env;
    pl::modules_registry.push_back(recordingEntry("unit_a"));

    auto p = makePipeline("unit_nolevel", {{"baseband", "", {}, ""}, {"soft", "unit_a", {}, ""}});

    std::string final_file = "untouched";
    p.run(env.input, env.out, {}, "not_a_level", false, nullptr, nullptr, &final_file);

    CHECK(callsOf("process").empty());
    CHECK(final_file == "");
}

TEST_CASE("Pipeline::run throws when a step's module is not registered", "[pipeline][pipeline_run]")
{
    RunEnv env;
    pl::modules_registry.push_back(recordingEntry("unit_a"));

    auto p = makePipeline("unit_unknown", {{"baseband", "", {}, ""}, {"soft", "unit_a", {}, ""}, {"frames", "unit_not_registered", {}, ""}});

    CHECK_THROWS_AS(p.run(env.input, env.out, {{"disable_multi_modules", true}}, "baseband"), satdump::satdump_exception_t);
    // Steps before the failure did run
    CHECK(processedIds() == std::vector<std::string>{"unit_a"});
}

TEST_CASE("Pipeline::run throws early when one of the two probed baseband modules is missing", "[pipeline][pipeline_run]")
{
    RunEnv env;
    pl::modules_registry.push_back(recordingEntry("unit_a"));

    auto p = makePipeline("unit_unknown2", {{"baseband", "", {}, ""}, {"soft", "unit_a", {}, ""}, {"frames", "unit_not_registered", {}, ""}});

    CHECK_THROWS_AS(p.run(env.input, env.out, {}, "baseband"), satdump::satdump_exception_t);
    CHECK(callsOf("construct").empty());
}

TEST_CASE("Pipeline::run fires PipelineDoneProcessingEvent and updates the UI call list", "[pipeline][pipeline_run]")
{
    RunEnv env;
    pl::modules_registry.push_back(recordingEntry("unit_a"));

    auto p = makePipeline("unit_event", {{"baseband", "", {}, ""}, {"soft", "unit_a", {}, ""}});

    auto events = std::make_shared<std::vector<pl::events::PipelineDoneProcessingEvent>>();
    satdump::eventBus->register_handler<pl::events::PipelineDoneProcessingEvent>([events](pl::events::PipelineDoneProcessingEvent e) { events->push_back(e); });

    auto ui_list = std::make_shared<std::vector<std::shared_ptr<pl::ProcessingModule>>>();
    auto ui_mtx = std::make_shared<std::mutex>();

    p.run(env.input, env.out, {}, "baseband", true, ui_list, ui_mtx);

    REQUIRE(events->size() == 1);
    CHECK((*events)[0].pipeline_id == "unit_event");
    CHECK((*events)[0].output_directory == env.out);

    // Modules are removed from the UI list once done
    CHECK(ui_list->empty());
    CHECK(processedIds() == std::vector<std::string>{"unit_a"});
}

TEST_CASE("Pipeline::run starts the products processor when a dataset is produced", "[pipeline][pipeline_run]")
{
    RunEnv env;
    pl::modules_registry.push_back(recordingEntry("unit_a"));
    pl::modules_registry.push_back(recordingEntry("products_processor"));

    auto p = makePipeline("unit_ds", {{"baseband", "", {}, ""}, {"soft", "unit_a", {}, ""}});
    writeFile(env.out + "/dataset.json", "{}");

    SECTION("enabled")
    {
        satdump::satdump_cfg.main_cfg["satdump_general"]["auto_process_products"]["value"] = true;
        p.run(env.input, env.out, {}, "baseband");

        auto proc = callsOf("process");
        REQUIRE(proc.size() == 2);
        CHECK(proc[1].id == "products_processor");
        CHECK(proc[1].input == env.out + "/dataset.json");
        CHECK(proc[1].hint == env.out + "/unit_ds");
    }

    SECTION("disabled")
    {
        satdump::satdump_cfg.main_cfg["satdump_general"]["auto_process_products"]["value"] = false;
        p.run(env.input, env.out, {}, "baseband");
        CHECK(processedIds() == std::vector<std::string>{"unit_a"});
    }
}

TEST_CASE("Pipeline::run processes a dataset.json input directly", "[pipeline][pipeline_run]")
{
    RunEnv env;
    pl::modules_registry.push_back(recordingEntry("unit_a"));
    pl::modules_registry.push_back(recordingEntry("products_processor"));
    satdump::satdump_cfg.main_cfg["satdump_general"]["auto_process_products"]["value"] = true;

    std::string dataset = env.dir.file("dataset.json");
    writeFile(dataset, "{}");

    auto p = makePipeline("unit_dsin", {{"baseband", "", {}, ""}, {"products", "unit_a", {}, ""}});
    p.run(dataset, env.out, {}, "products");

    auto proc = callsOf("process");
    REQUIRE(proc.size() == 1);
    CHECK(proc[0].id == "products_processor");
    CHECK(proc[0].input == dataset);
}

// pipeline_run.cpp:137 : levels without a module (e.g. the trailing "products": {} placeholder used
// by many shipped pipelines such as "adityal1_tlm_link") are not skipped, so run() throws
// "Module  is not registered" after the real work is done, and never sets final_file.
TEST_CASE("Pipeline::run tolerates a trailing level without a module", "[pipeline][pipeline_run][!shouldfail]")
{
    RunEnv env;
    pl::modules_registry.push_back(recordingEntry("unit_a"));

    auto p = makePipeline("unit_trailing", {{"baseband", "", {}, ""}, {"frames", "unit_a", {}, ""}, {"products", "", {}, ""}});

    std::string final_file;
    CHECK_NOTHROW(p.run(env.input, env.out, {}, "baseband", false, nullptr, nullptr, &final_file));
    CHECK(final_file == env.out + "/unit_trailing_unit_a.bin");
}

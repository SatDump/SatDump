#include "pipeline/pipeline.h"
#include "pipeline/pipeline_test_helpers.h"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <map>
#include <set>

using namespace pipeline_test;
namespace pl = satdump::pipeline;

namespace
{
    const std::string PIPELINES_DIR = std::string(SATDUMP_TEST_SOURCE_DIR) + "/resources/pipelines";

    std::vector<std::string> pipelineFiles()
    {
        std::vector<std::string> files;
        for (auto &e : std::filesystem::recursive_directory_iterator(PIPELINES_DIR))
            if (e.is_regular_file() && e.path().extension() == ".json")
                files.push_back(e.path().string());
        std::sort(files.begin(), files.end());
        return files;
    }

    // Loads the real pipeline definitions with every referenced module registered
    // as a placeholder, so that all of them get parsed.
    struct RealPipelines
    {
        TempDir cwd;
        TempDir user;
        PipelinesStateGuard state;
        ModuleRegistryGuard reg;
        CwdGuard cwd_guard;
        UserPathGuard user_guard;
        std::set<std::string> module_names;

        RealPipelines() : cwd("pipe_res_cwd"), user("pipe_res_user"), cwd_guard(cwd.path), user_guard(user.str())
        {
            // First pass: with no modules registered, nothing can be parsed
            pl::loadPipelines(PIPELINES_DIR);

            for (auto &p : pl::pipelines_system_json.items())
                for (auto &s : p.value()["work"].items())
                    if (s.value().contains("module"))
                        module_names.insert(s.value()["module"].get<std::string>());

            for (auto &m : module_names)
                pl::modules_registry.push_back(recordingEntry(m));

            pl::pipelines_json = pl::pipelines_system_json;
            pl::parsePipelines();
        }
    };

    bool isInteger(const nlohmann::ordered_json &v)
    {
        // Several integer parameters are written as e.g. 1e6, which nlohmann parses as a float
        return v.is_number_integer() || (v.is_number_float() && v.get<double>() == (double)(int64_t)v.get<double>());
    }
} // namespace

TEST_CASE("resources/pipelines: every file is valid JSON (with comments) and ids are unique", "[pipeline][resources]")
{
    auto files = pipelineFiles();
    REQUIRE(files.size() > 50);

    std::map<std::string, std::string> ids;
    for (auto &f : files)
    {
        INFO(f);
        nlohmann::ordered_json j;
        REQUIRE_NOTHROW(j = nlohmann::ordered_json::parse(readFile(f)));
        REQUIRE(j.is_object());
        CHECK(j.size() > 0);
        for (auto &p : j.items())
        {
            INFO(p.key());
            CHECK(ids.count(p.key()) == 0);
            ids[p.key()] = f;
        }
    }
    CHECK(ids.size() > 200);
}

TEST_CASE("pipeline::loadPipelines loads every shipped pipeline", "[pipeline][resources]")
{
    RealPipelines real;

    // Independent count of the top-level ids in every file
    size_t expected = 0;
    for (auto &f : pipelineFiles())
        expected += nlohmann::ordered_json::parse(readFile(f)).size();

    CHECK(pl::pipelines_system_json.size() == expected);
    CHECK(real.module_names.size() > 50);

    // Every pipeline references at least one module, so all of them parse
    CHECK(pl::pipelines.size() == expected);
}

TEST_CASE("resources/pipelines: parsed pipelines have names, ids, and a well-formed step list", "[pipeline][resources]")
{
    RealPipelines real;
    REQUIRE(pl::pipelines.size() > 200);

    std::set<std::string> seen;
    for (auto &p : pl::pipelines)
    {
        INFO(p.id);
        CHECK_FALSE(p.id.empty());
        CHECK_FALSE(p.name.empty());
        CHECK(seen.insert(p.id).second);
        CHECK(pl::getPipelineFromID(p.id).name == p.name);

        auto &raw = pl::pipelines_system_json[p.id];
        REQUIRE(p.steps.size() == raw["work"].size());
        REQUIRE(p.steps.size() >= 2);

        // The first level is the input level and never runs a module
        CHECK(p.steps[0].module.empty());

        bool has_module = false;
        std::set<std::string> levels;
        for (auto &s : p.steps)
        {
            INFO(s.level);
            CHECK_FALSE(s.level.empty());
            CHECK(levels.insert(s.level).second);
            if (!s.module.empty())
            {
                has_module = true;
                CHECK(pl::moduleExists(s.module));
            }
            if (!s.parameters.is_null())
                CHECK(s.parameters.is_object());
        }
        CHECK(has_module);
    }
}

TEST_CASE("resources/pipelines: pipelines are sorted by case-insensitive name", "[pipeline][resources]")
{
    RealPipelines real;

    auto lower = [](std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    };
    for (size_t i = 1; i < pl::pipelines.size(); i++)
    {
        INFO(pl::pipelines[i - 1].name << " / " << pl::pipelines[i].name);
        CHECK_FALSE(lower(pl::pipelines[i].name) < lower(pl::pipelines[i - 1].name));
    }
}

TEST_CASE("resources/pipelines: live configurations reference existing steps", "[pipeline][resources]")
{
    RealPipelines real;

    int live_count = 0;
    for (auto &p : pl::pipelines)
    {
        INFO(p.id);
        CHECK(p.live == pl::pipelines_system_json[p.id].contains("live"));
        if (!p.live)
            continue;
        live_count++;

        CHECK_FALSE(p.live_cfg.normal_live.empty());
        for (auto *list : {&p.live_cfg.normal_live, &p.live_cfg.server_live, &p.live_cfg.client_live})
            for (int idx : *list)
            {
                CHECK(idx >= 0);
                CHECK(idx < (int)p.steps.size());
            }

        // The normal live chain must be contiguous, starting right after the input level
        for (size_t i = 0; i < p.live_cfg.normal_live.size(); i++)
            CHECK(p.live_cfg.normal_live[i] == (int)i + 1);

        for (int idx : p.live_cfg.normal_live)
            CHECK_FALSE(p.steps[idx].module.empty());

        if (!p.live_cfg.server_live.empty() || !p.live_cfg.client_live.empty())
            CHECK(p.live_cfg.pkt_size > 0);
    }
    CHECK(live_count > 100);
}

// Bug: resources/pipelines/GOES.json:211 : the "goes_grb" client configuration is [3, 0], and step 0 is
// the "baseband" input level which has no module, so LivePipeline::start_client() would fail with
// "Could not find module".
TEST_CASE("resources/pipelines: server and client live steps all have modules", "[pipeline][resources][!shouldfail]")
{
    RealPipelines real;

    for (auto &p : pl::pipelines)
    {
        INFO(p.id);
        for (int idx : p.live_cfg.server_live)
            CHECK_FALSE(p.steps[idx].module.empty());
        for (int idx : p.live_cfg.client_live)
            CHECK_FALSE(p.steps[idx].module.empty());
    }
}

TEST_CASE("resources/pipelines: preset frequencies and samplerates are sane", "[pipeline][resources]")
{
    RealPipelines real;

    for (auto &p : pl::pipelines)
    {
        INFO(p.id);
        for (auto &f : p.preset.frequencies)
        {
            INFO(f.first);
            CHECK_FALSE(f.first.empty());
            // Everything shipped is between VHF and Ka band
            CHECK(f.second >= 10000000ULL);
            CHECK(f.second <= 40000000000ULL);
        }

        auto &params = pl::pipelines_system_json[p.id]["parameters"];
        if (params.contains("samplerate") && params["samplerate"].contains("value"))
        {
            CHECK(p.preset.samplerate == params["samplerate"]["value"].get<uint64_t>());
            CHECK(p.preset.samplerate > 0);
        }
    }
}

TEST_CASE("resources/pipelines: typed parameters have a value of the declared type", "[pipeline][resources]")
{
    RealPipelines real;

    const std::set<std::string> known = {"bool", "int", "float", "string", "options", "timestamp", "notated_int"};
    int typed = 0;
    for (auto &p : pl::pipelines)
    {
        for (auto &param : p.editable_parameters.items())
        {
            INFO(p.id << " / " << param.key());
            auto &v = param.value();
            REQUIRE(v.is_object());
            if (!v.contains("type"))
            {
                // Untyped entries are only used for the samplerate preset
                CHECK(param.key() == "samplerate");
                continue;
            }
            typed++;

            std::string type = v["type"];
            CHECK(known.count(type) == 1);
            REQUIRE(v.contains("value"));
            auto &val = v["value"];

            if (type == "bool")
                CHECK(val.is_boolean());
            else if (type == "int" || type == "timestamp" || type == "notated_int")
                CHECK(isInteger(val));
            else if (type == "float")
                CHECK(val.is_number());
            else if (type == "string")
                CHECK(val.is_string());
            else if (type == "options")
            {
                CHECK(val.is_string());
                REQUIRE(v.contains("options"));
                CHECK(v["options"].is_array());
                CHECK(v["options"].size() >= 2);
            }

            if (v.contains("name"))
                CHECK(v["name"].is_string());
        }
    }
    CHECK(typed > 100);
}

// Bug: resources/pipelines/Others.json:20 : "sstv_decoder" declares "sstv_mode" with the default value
// "M2-2" (copied from the METEOR pipelines), which is not one of its options BW8/FAX480/Robot36/PD120.
TEST_CASE("resources/pipelines: option parameter defaults are one of their options", "[pipeline][resources][!shouldfail]")
{
    RealPipelines real;

    for (auto &p : pl::pipelines)
        for (auto &param : p.editable_parameters.items())
        {
            auto &v = param.value();
            if (!v.contains("type") || v["type"] != "options")
                continue;
            INFO(p.id << " / " << param.key());
            auto opts = v["options"].get<std::vector<std::string>>();
            CHECK(std::find(opts.begin(), opts.end(), v["value"].get<std::string>()) != opts.end());
        }
}

// Bug: resources/pipelines/Work-In-Progress.json:213 : the "cfosat1_dump" "soft" step uses the key
// "psk_demod" instead of "module": "psk_demod", so the step silently has no module.
TEST_CASE("resources/pipelines: steps only use the 'module' and 'parameters' keys", "[pipeline][resources][!shouldfail]")
{
    RealPipelines real;

    for (auto &p : pl::pipelines_system_json.items())
        for (auto &s : p.value()["work"].items())
            for (auto &k : s.value().items())
            {
                INFO(p.key() << " / " << s.key() << " / " << k.key());
                CHECK((k.key() == "module" || k.key() == "parameters"));
            }
}

#include "core/exception.h"
#include "nlohmann/json_utils.h"
#include "pipeline/pipeline.h"
#include "pipeline_test_helpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace pipeline_test_helpers;
namespace pl = satdump::pipeline;

namespace
{
    // Registers the given module names as (never-run) registry entries
    void registerFakes(const std::vector<std::string> &ids)
    {
        for (auto &id : ids)
            pl::modules_registry.push_back(recordingEntry(id));
    }

    const char *TINY_PIPELINES = R"({
        "unit_tiny": {
            "name": "Tiny Test Pipeline",
            "live": [1, 2],
            "frequencies": [["Main", 137100000], ["Backup", 137900000]],
            "parameters": {
                "samplerate": { "value": 6e6 },
                "flag": { "type": "bool", "value": true, "name": "A flag" }
            },
            "work": {
                "baseband": {},
                "soft": {
                    "module": "unit_mod_demod",
                    "parameters": { "symbolrate": 72000, "rrc_alpha": 0.5 }
                },
                "frames": {
                    "module": "unit_mod_decoder",
                    "parameters": { "input_override": "custom.soft", "x": "y" }
                },
                "products": { "module": "unit_mod_products" }
            }
        }
    })";
} // namespace

TEST_CASE("pipeline::parsePipelines parses every field of a pipeline definition", "[pipeline][pipeline]")
{
    PipelinesStateGuard state;
    ModuleRegistryGuard reg;
    registerFakes({"unit_mod_demod", "unit_mod_decoder", "unit_mod_products"});

    pl::pipelines_json = nlohmann::ordered_json::parse(TINY_PIPELINES);
    pl::parsePipelines();

    REQUIRE(pl::pipelines.size() == 1);
    const pl::Pipeline &p = pl::pipelines[0];

    CHECK(p.id == "unit_tiny");
    CHECK(p.name == "Tiny Test Pipeline");
    CHECK(p.editable_parameters["flag"]["type"] == "bool");
    CHECK(p.editable_parameters["flag"]["value"] == true);

    CHECK(p.live);
    CHECK(p.live_cfg.normal_live == std::vector<int>{1, 2});
    CHECK(p.live_cfg.server_live.empty());
    CHECK(p.live_cfg.client_live.empty());
    CHECK(p.live_cfg.pkt_size == -1);

    CHECK(p.preset.samplerate == 6000000);
    REQUIRE(p.preset.frequencies.size() == 2);
    CHECK(p.preset.frequencies[0].first == "Main");
    CHECK(p.preset.frequencies[0].second == 137100000);
    CHECK(p.preset.frequencies[1].first == "Backup");
    CHECK(p.preset.frequencies[1].second == 137900000);

    REQUIRE(p.steps.size() == 4);
    CHECK(p.steps[0].level == "baseband");
    CHECK(p.steps[0].module == "");
    CHECK(p.steps[0].parameters.is_null());
    CHECK(p.steps[0].input_override == "");

    CHECK(p.steps[1].level == "soft");
    CHECK(p.steps[1].module == "unit_mod_demod");
    CHECK(p.steps[1].parameters["symbolrate"] == 72000);
    CHECK(p.steps[1].parameters["rrc_alpha"] == 0.5);
    CHECK(p.steps[1].input_override == "");

    CHECK(p.steps[2].level == "frames");
    CHECK(p.steps[2].module == "unit_mod_decoder");
    CHECK(p.steps[2].input_override == "custom.soft");
    CHECK(p.steps[2].parameters["x"] == "y");

    CHECK(p.steps[3].level == "products");
    CHECK(p.steps[3].module == "unit_mod_products");
    CHECK(p.steps[3].parameters.is_null());
}

TEST_CASE("pipeline::parsePipelines handles the object form of live configurations", "[pipeline][pipeline]")
{
    PipelinesStateGuard state;
    ModuleRegistryGuard reg;
    registerFakes({"unit_mod_a", "unit_mod_b", "unit_mod_c"});

    pl::pipelines_json = nlohmann::ordered_json::parse(R"({
        "unit_live_obj": {
            "name": "Live Object",
            "live": { "pkt_size": 10240, "default": [1, 2, 3], "server": [1, 2], "client": [3] },
            "work": { "baseband": {}, "soft": { "module": "unit_mod_a" }, "cadu": { "module": "unit_mod_b" }, "products": { "module": "unit_mod_c" } }
        },
        "unit_live_default_only": {
            "name": "Live Default Only",
            "live": { "default": [1] },
            "work": { "baseband": {}, "soft": { "module": "unit_mod_a" } }
        },
        "unit_not_live": {
            "name": "Not Live",
            "work": { "baseband": {}, "soft": { "module": "unit_mod_a" } }
        }
    })");
    pl::parsePipelines();

    REQUIRE(pl::pipelines.size() == 3);

    auto obj = pl::getPipelineFromID("unit_live_obj");
    CHECK(obj.live);
    CHECK(obj.live_cfg.normal_live == std::vector<int>{1, 2, 3});
    CHECK(obj.live_cfg.server_live == std::vector<int>{1, 2});
    CHECK(obj.live_cfg.client_live == std::vector<int>{3});
    CHECK(obj.live_cfg.pkt_size == 10240);

    auto def = pl::getPipelineFromID("unit_live_default_only");
    CHECK(def.live);
    CHECK(def.live_cfg.normal_live == std::vector<int>{1});
    CHECK(def.live_cfg.server_live.empty());
    CHECK(def.live_cfg.client_live.empty());
    CHECK(def.live_cfg.pkt_size == -1);

    auto nl = pl::getPipelineFromID("unit_not_live");
    CHECK_FALSE(nl.live);
    CHECK(nl.live_cfg.normal_live.empty());
    CHECK(nl.preset.frequencies.empty());
}

TEST_CASE("pipeline::parsePipelines sets a zero preset samplerate when no value is given", "[pipeline][pipeline]")
{
    PipelinesStateGuard state;
    ModuleRegistryGuard reg;
    registerFakes({"unit_mod_a"});

    pl::pipelines_json = nlohmann::ordered_json::parse(R"({
        "unit_sr": { "name": "SR", "parameters": { "samplerate": { "type": "int" } }, "work": { "baseband": {}, "soft": { "module": "unit_mod_a" } } }
    })");
    pl::parsePipelines();

    REQUIRE(pl::pipelines.size() == 1);
    CHECK(pl::pipelines[0].preset.samplerate == 0);
}

TEST_CASE("pipeline::parsePipelines skips pipelines with unregistered or no modules", "[pipeline][pipeline]")
{
    PipelinesStateGuard state;
    ModuleRegistryGuard reg;
    registerFakes({"unit_mod_known"});

    pl::pipelines_json = nlohmann::ordered_json::parse(R"({
        "unit_ok": { "name": "OK", "work": { "baseband": {}, "soft": { "module": "unit_mod_known" } } },
        "unit_missing": { "name": "Missing", "work": { "baseband": {}, "soft": { "module": "unit_mod_known" }, "frames": { "module": "unit_mod_not_loaded" } } },
        "unit_nomodule": { "name": "No Module", "work": { "baseband": {}, "soft": {} } },
        "unit_nowork": { "name": "No Work" }
    })");
    pl::parsePipelines();

    REQUIRE(pl::pipelines.size() == 1);
    CHECK(pl::pipelines[0].id == "unit_ok");
    CHECK_THROWS_AS(pl::getPipelineFromID("unit_missing"), satdump::satdump_exception_t);
    CHECK_THROWS_AS(pl::getPipelineFromID("unit_nomodule"), satdump::satdump_exception_t);
    CHECK_THROWS_AS(pl::getPipelineFromID("unit_nowork"), satdump::satdump_exception_t);
}

TEST_CASE("pipeline::parsePipelines skips malformed pipelines without affecting others", "[pipeline][pipeline]")
{
    PipelinesStateGuard state;
    ModuleRegistryGuard reg;
    registerFakes({"unit_mod_known"});

    pl::pipelines_json = nlohmann::ordered_json::parse(R"({
        "unit_bad_name": { "name": 42, "work": { "baseband": {}, "soft": { "module": "unit_mod_known" } } },
        "unit_bad_live": { "name": "Bad live", "live": "yes", "work": { "baseband": {}, "soft": { "module": "unit_mod_known" } } },
        "unit_bad_freq": { "name": "Bad freq", "frequencies": [["A"]], "work": { "baseband": {}, "soft": { "module": "unit_mod_known" } } },
        "unit_good": { "name": "Good", "work": { "baseband": {}, "soft": { "module": "unit_mod_known" } } }
    })");
    pl::parsePipelines();

    REQUIRE(pl::pipelines.size() == 1);
    CHECK(pl::pipelines[0].id == "unit_good");
}

TEST_CASE("pipeline::parsePipelines sorts pipelines by name, case-insensitively", "[pipeline][pipeline]")
{
    PipelinesStateGuard state;
    ModuleRegistryGuard reg;
    registerFakes({"unit_mod"});

    pl::pipelines_json = nlohmann::ordered_json::parse(R"({
        "id1": { "name": "charlie", "work": { "baseband": {}, "s": { "module": "unit_mod" } } },
        "id2": { "name": "Alpha", "work": { "baseband": {}, "s": { "module": "unit_mod" } } },
        "id3": { "name": "bravo", "work": { "baseband": {}, "s": { "module": "unit_mod" } } },
        "id4": { "name": "Delta", "work": { "baseband": {}, "s": { "module": "unit_mod" } } }
    })");
    pl::parsePipelines();

    REQUIRE(pl::pipelines.size() == 4);
    CHECK(pl::pipelines[0].name == "Alpha");
    CHECK(pl::pipelines[1].name == "bravo");
    CHECK(pl::pipelines[2].name == "charlie");
    CHECK(pl::pipelines[3].name == "Delta");
}

TEST_CASE("pipeline::parsePipelines replaces previously parsed pipelines", "[pipeline][pipeline]")
{
    PipelinesStateGuard state;
    ModuleRegistryGuard reg;
    registerFakes({"unit_mod"});

    pl::pipelines_json = nlohmann::ordered_json::parse(R"({ "first": { "name": "First", "work": { "baseband": {}, "s": { "module": "unit_mod" } } } })");
    pl::parsePipelines();
    REQUIRE(pl::pipelines.size() == 1);

    pl::pipelines_json = nlohmann::ordered_json::parse(R"({ "second": { "name": "Second", "work": { "baseband": {}, "s": { "module": "unit_mod" } } } })");
    pl::parsePipelines();
    REQUIRE(pl::pipelines.size() == 1);
    CHECK(pl::pipelines[0].id == "second");
}

TEST_CASE("pipeline::getPipelineFromID returns a copy or throws", "[pipeline][pipeline]")
{
    PipelinesStateGuard state;
    ModuleRegistryGuard reg;
    registerFakes({"unit_mod_demod", "unit_mod_decoder", "unit_mod_products"});

    pl::pipelines_json = nlohmann::ordered_json::parse(TINY_PIPELINES);
    pl::parsePipelines();

    pl::Pipeline p = pl::getPipelineFromID("unit_tiny");
    CHECK(p.name == "Tiny Test Pipeline");
    p.name = "modified";
    CHECK(pl::pipelines[0].name == "Tiny Test Pipeline");

    CHECK_THROWS_AS(pl::getPipelineFromID("unit_tiny_nope"), satdump::satdump_exception_t);
    CHECK_THROWS_AS(pl::getPipelineFromID(""), satdump::satdump_exception_t);
}

TEST_CASE("Pipeline::prepareParameters merges pipeline parameters over module parameters", "[pipeline][pipeline]")
{
    nlohmann::json module_params = {{"symbolrate", 1000}, {"rrc_alpha", 0.5}, {"nested", {{"a", 1}}}};
    nlohmann::json pipeline_params = {{"rrc_alpha", 0.35}, {"extra", "x"}, {"nested", {{"b", 2}}}};

    nlohmann::json out = pl::Pipeline::prepareParameters(module_params, pipeline_params);

    CHECK(out["symbolrate"] == 1000);
    CHECK(out["rrc_alpha"] == 0.35);
    CHECK(out["extra"] == "x");
    // Values are replaced wholesale, not deep-merged
    CHECK(out["nested"] == nlohmann::json{{"b", 2}});
    CHECK(out.size() == 4);

    // Inputs are not modified
    CHECK(module_params["rrc_alpha"] == 0.5);
    CHECK(module_params.size() == 3);
}

TEST_CASE("Pipeline::prepareParameters handles null module parameters", "[pipeline][pipeline]")
{
    nlohmann::json module_params; // null, as for a step without "parameters"
    nlohmann::json pipeline_params = {{"a", 1}};
    nlohmann::json empty;

    CHECK(pl::Pipeline::prepareParameters(module_params, pipeline_params) == nlohmann::json{{"a", 1}});
    CHECK(pl::Pipeline::prepareParameters(module_params, empty).is_null());
}

////////////////////////////////////////////////////////////
// loadPipelines / savePipelines (file based)
////////////////////////////////////////////////////////////

namespace
{
    // Isolates loadPipelines from any pipelines.json in the current folder or user path
    struct LoadEnv
    {
        TempDir root;
        TempDir cwd;
        TempDir user;
        PipelinesStateGuard state;
        ModuleRegistryGuard reg;
        CwdGuard cwd_guard;
        UserPathGuard user_guard;

        LoadEnv() : root("pipe_load_sys"), cwd("pipe_load_cwd"), user("pipe_load_user"), cwd_guard(cwd.path), user_guard(user.str()) { registerFakes({"unit_mod"}); }
    };

    std::string simplePipeline(const std::string &id, const std::string &name)
    {
        return "\"" + id + "\": { \"name\": \"" + name + "\", \"work\": { \"baseband\": {}, \"soft\": { \"module\": \"unit_mod\" } } }";
    }
} // namespace

TEST_CASE("pipeline::loadPipelines loads all .json files recursively", "[pipeline][pipeline][load]")
{
    LoadEnv env;
    writeFile(env.root.file("a.json"), "{" + simplePipeline("unit_a", "A") + "}");
    writeFile(env.root.file("sub/dir/b.json"), "{" + simplePipeline("unit_b", "B") + "," + simplePipeline("unit_c", "C") + "}");
    writeFile(env.root.file("notes.txt"), "{" + simplePipeline("unit_txt", "TXT") + "}");
    // .json.inc files are only meant to be included by others
    writeFile(env.root.file("common.json.inc"), "{" + simplePipeline("unit_inc", "INC") + "}");

    pl::loadPipelines(env.root.str());

    CHECK(pl::pipelines_system_json.size() == 3);
    // Not compared for equality: parsePipelines() uses operator[] and adds "parameters": null entries
    CHECK(pl::pipelines_json.size() == 3);
    CHECK(pl::pipelines_json["unit_b"]["name"] == "B");
    REQUIRE(pl::pipelines.size() == 3);
    CHECK(pl::pipelines[0].id == "unit_a");
    CHECK(pl::pipelines[1].id == "unit_b");
    CHECK(pl::pipelines[2].id == "unit_c");
    CHECK_FALSE(pl::pipelines_system_json.contains("unit_txt"));
    CHECK_FALSE(pl::pipelines_system_json.contains("unit_inc"));

    // Without a user file, saving would go to the user path
    CHECK(pl::user_cfg_path == env.user.str() + "/pipelines.json");
}

TEST_CASE("pipeline::loadPipelines lets later files (by path order) override earlier ids", "[pipeline][pipeline][load]")
{
    LoadEnv env;
    writeFile(env.root.file("a.json"), "{" + simplePipeline("unit_dup", "From A") + "}");
    writeFile(env.root.file("b.json"), "{" + simplePipeline("unit_dup", "From B") + "}");

    pl::loadPipelines(env.root.str());

    REQUIRE(pl::pipelines.size() == 1);
    CHECK(pl::pipelines[0].name == "From B");
}

TEST_CASE("pipeline::loadPipelines skips invalid files and accepts comments", "[pipeline][pipeline][load]")
{
    LoadEnv env;
    writeFile(env.root.file("a_broken.json"), "{ this is not json ");
    writeFile(env.root.file("b_ok.json"), "{\n// A comment\n" + simplePipeline("unit_ok", "OK") + " /* block */ }");

    pl::loadPipelines(env.root.str());

    REQUIRE(pl::pipelines.size() == 1);
    CHECK(pl::pipelines[0].id == "unit_ok");
}

TEST_CASE("pipeline::loadPipeline substitutes .json.inc includes", "[pipeline][pipeline][load]")
{
    LoadEnv env;
    writeFile(env.root.file("params.json.inc"), R"({ "symbolrate": 72000, "constellation": "bpsk" })");
    writeFile(env.root.file("main.json"), R"({
        "unit_inc_a": { "name": "Inc A", "work": { "baseband": {}, "soft": { "module": "unit_mod", "parameters": "params.json.inc" } } },
        "unit_inc_b": { "name": "Inc B", "work": { "baseband": {}, "soft": { "module": "unit_mod", "parameters": "params.json.inc" } } },
        "unit_inc_missing": { "name": "Inc Missing", "work": { "baseband": {}, "soft": { "module": "unit_mod", "parameters": "nothere.json.inc" } } }
    })");

    pl::loadPipelines(env.root.str());

    REQUIRE(pl::pipelines.size() == 3);
    auto a = pl::getPipelineFromID("unit_inc_a");
    CHECK(a.steps[1].parameters == nlohmann::json{{"symbolrate", 72000}, {"constellation", "bpsk"}});
    auto b = pl::getPipelineFromID("unit_inc_b");
    CHECK(b.steps[1].parameters == a.steps[1].parameters);

    // A missing include is left as the literal string
    auto m = pl::getPipelineFromID("unit_inc_missing");
    CHECK(m.steps[1].parameters == "nothere.json.inc");
}

TEST_CASE("pipeline::loadPipelines merges user pipeline diffs from the user path", "[pipeline][pipeline][load]")
{
    LoadEnv env;
    writeFile(env.root.file("sys.json"), "{" + simplePipeline("unit_keep", "Keep") + "," + simplePipeline("unit_rename", "Old Name") + "," + simplePipeline("unit_delete", "Delete") + "}");
    writeFile(env.user.file("pipelines.json"), R"({
        "unit_rename": { "name": "New Name" },
        "unit_delete": null,
        "unit_user_new": { "name": "User New", "work": { "baseband": {}, "soft": { "module": "unit_mod" } } }
    })");

    pl::loadPipelines(env.root.str());

    CHECK(pl::user_cfg_path == env.user.str() + "/pipelines.json");
    CHECK(pl::pipelines_system_json.size() == 3);
    CHECK(pl::pipelines_system_json["unit_rename"]["name"] == "Old Name");

    REQUIRE(pl::pipelines.size() == 3);
    CHECK(pl::getPipelineFromID("unit_rename").name == "New Name");
    CHECK(pl::getPipelineFromID("unit_rename").steps.size() == 2);
    CHECK(pl::getPipelineFromID("unit_keep").name == "Keep");
    CHECK(pl::getPipelineFromID("unit_user_new").name == "User New");
    CHECK_THROWS(pl::getPipelineFromID("unit_delete"));
}

TEST_CASE("pipeline::loadPipelines prefers pipelines.json in the current directory", "[pipeline][pipeline][load]")
{
    LoadEnv env;
    writeFile(env.root.file("sys.json"), "{" + simplePipeline("unit_p", "System") + "}");
    writeFile(env.user.file("pipelines.json"), R"({ "unit_p": { "name": "From User Path" } })");
    writeFile(env.cwd.file("pipelines.json"), R"({ "unit_p": { "name": "From Cwd" } })");

    pl::loadPipelines(env.root.str());

    CHECK(pl::user_cfg_path == "pipelines.json");
    REQUIRE(pl::pipelines.size() == 1);
    CHECK(pl::pipelines[0].name == "From Cwd");
}

TEST_CASE("pipeline::loadPipelines falls back to system pipelines on an invalid user file", "[pipeline][pipeline][load]")
{
    LoadEnv env;
    writeFile(env.root.file("sys.json"), "{" + simplePipeline("unit_p", "System") + "}");
    writeFile(env.user.file("pipelines.json"), "{ broken");

    pl::loadPipelines(env.root.str());

    REQUIRE(pl::pipelines.size() == 1);
    CHECK(pl::pipelines[0].name == "System");
}

TEST_CASE("pipeline::savePipelines writes only the user diff", "[pipeline][pipeline][load]")
{
    LoadEnv env;
    writeFile(env.root.file("sys.json"), "{" + simplePipeline("unit_a", "A") + "," + simplePipeline("unit_b", "B") + "}");

    pl::loadPipelines(env.root.str());
    REQUIRE(pl::pipelines.size() == 2);

    pl::pipelines_json["unit_a"]["name"] = "A edited";
    pl::pipelines_json.erase("unit_b");
    pl::savePipelines();

    // In-memory pipelines were re-parsed
    REQUIRE(pl::pipelines.size() == 1);
    CHECK(pl::pipelines[0].name == "A edited");

    std::string saved = env.user.file("pipelines.json");
    REQUIRE(std::filesystem::exists(saved));
    nlohmann::ordered_json diff = loadJsonFile(saved);
    CHECK(diff == nlohmann::ordered_json::parse(R"({ "unit_a": { "name": "A edited" }, "unit_b": null })"));

    // Loading again applies the saved diff
    pl::loadPipelines(env.root.str());
    REQUIRE(pl::pipelines.size() == 1);
    CHECK(pl::pipelines[0].name == "A edited");
}

TEST_CASE("pipeline::savePipelines creates the user directory if needed", "[pipeline][pipeline][load]")
{
    LoadEnv env;
    UserPathGuard nested(env.user.str() + "/deeper/dir");
    writeFile(env.root.file("sys.json"), "{" + simplePipeline("unit_a", "A") + "}");

    pl::loadPipelines(env.root.str());
    pl::pipelines_json["unit_a"]["name"] = "Changed";
    pl::savePipelines();

    CHECK(std::filesystem::exists(env.user.str() + "/deeper/dir/pipelines.json"));
}

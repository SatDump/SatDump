#include "core/config.h"
#include "helpers/guards.h"
#include "helpers/temp_files.h"
#include "nlohmann/json_utils.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <filesystem>
#include <fstream>

// Every test uses its own SatDumpConfigHandler, never the global satdump::satdump_cfg. loadUser()
// looks for ./settings.json first, so tests run from an empty temporary directory (restored after).

namespace
{
    using test_helpers::CwdGuard;
    using test_helpers::TempDir;
    using test_helpers::writeFile;

    nlohmann::ordered_json readJson(const std::filesystem::path &p)
    {
        std::ifstream f(p);
        return nlohmann::ordered_json::parse(f);
    }

    nlohmann::ordered_json defaults()
    {
        return nlohmann::ordered_json::parse(R"({
            "satdump_general": {
                "auto_process_products": {"type": "bool", "value": true},
                "kepler_update_interval": {"type": "options", "value": "1 day"},
                "buffer": {"type": "int", "value": 1024}
            },
            "satdump_directories": {
                "recording_path": {"type": "string", "value": "/data/rec"}
            },
            "user_interface": {
                "play_audio": {"type": "bool", "value": false},
                "theme": {"type": "string", "value": "Dark"}
            },
            "plugin_settings": {"a": 1, "b": 2}
        })");
    }

    // A temporary working directory, a defaults file and a user directory
    struct ConfigEnv
    {
        TempDir root{"env"};
        std::filesystem::path cwd = root.path / "cwd";
        std::filesystem::path user = root.path / "user";
        std::filesystem::path defaults_file = root.path / "satdump_cfg.json";
        std::unique_ptr<CwdGuard> cg;

        ConfigEnv()
        {
            std::filesystem::create_directories(cwd);
            std::filesystem::create_directories(user);
            writeFile(defaults_file, defaults().dump(4));
            cg = std::make_unique<CwdGuard>(cwd);
        }
    };
} // namespace

TEST_CASE("SatDumpConfigHandler load without user settings keeps the defaults", "[core][config]")
{
    ConfigEnv env;
    satdump::SatDumpConfigHandler cfg;
    cfg.load(env.defaults_file.string(), env.user.string());

    CHECK(cfg.default_cfg == defaults());
    CHECK(cfg.main_cfg == defaults());
    CHECK(cfg.user_cfg_path == env.user.string() + "/settings.json");
}

TEST_CASE("SatDumpConfigHandler load applies the user settings diff", "[core][config]")
{
    ConfigEnv env;
    writeFile(env.user / "settings.json", R"({
        "satdump_general": {"buffer": {"value": 4096}, "new_key": {"value": "x"}},
        "plugin_settings": {"a": null}
    })");

    satdump::SatDumpConfigHandler cfg;
    cfg.load(env.defaults_file.string(), env.user.string());

    CHECK(cfg.user_cfg_path == env.user.string() + "/settings.json");
    CHECK(cfg.default_cfg == defaults());
    // Changed values override, untouched siblings stay
    CHECK(cfg.main_cfg["satdump_general"]["buffer"]["value"] == 4096);
    CHECK(cfg.main_cfg["satdump_general"]["buffer"]["type"] == "int");
    CHECK(cfg.main_cfg["satdump_general"]["new_key"]["value"] == "x");
    CHECK(cfg.main_cfg["satdump_general"]["auto_process_products"]["value"] == true);
    // A null in the diff removes the key
    CHECK_FALSE(cfg.main_cfg["plugin_settings"].contains("a"));
    CHECK(cfg.main_cfg["plugin_settings"]["b"] == 2);
}

TEST_CASE("SatDumpConfigHandler prefers settings.json in the current directory", "[core][config]")
{
    ConfigEnv env;
    writeFile(env.user / "settings.json", R"({"user_interface": {"theme": {"value": "FromUserDir"}}})");
    writeFile(env.cwd / "settings.json", R"({"user_interface": {"theme": {"value": "FromCwd"}}})");

    satdump::SatDumpConfigHandler cfg;
    cfg.load(env.defaults_file.string(), env.user.string());
    CHECK(cfg.user_cfg_path == "settings.json");
    CHECK(cfg.main_cfg["user_interface"]["theme"]["value"] == "FromCwd");
}

TEST_CASE("SatDumpConfigHandler saveUser stores only the diff and reloads it", "[core][config]")
{
    ConfigEnv env;
    // The user directory does not exist yet; saveUser creates it
    std::filesystem::path user = env.root.path / "fresh" / "nested";

    {
        satdump::SatDumpConfigHandler cfg;
        cfg.load(env.defaults_file.string(), user.string());
        cfg.main_cfg["user_interface"]["theme"]["value"] = "Light";
        cfg.main_cfg["plugin_settings"].erase("b");
        cfg.main_cfg["extra"] = {{"k", 1}};
        cfg.saveUser();
    }

    REQUIRE(std::filesystem::exists(user / "settings.json"));
    auto saved = readJson(user / "settings.json");
    CHECK(saved == nlohmann::ordered_json::parse(R"({
        "user_interface": {"theme": {"value": "Light"}},
        "plugin_settings": {"b": null},
        "extra": {"k": 1}
    })"));

    satdump::SatDumpConfigHandler reloaded;
    reloaded.load(env.defaults_file.string(), user.string());
    CHECK(reloaded.main_cfg["user_interface"]["theme"]["value"] == "Light");
    CHECK(reloaded.main_cfg["user_interface"]["play_audio"]["value"] == false);
    CHECK_FALSE(reloaded.main_cfg["plugin_settings"].contains("b"));
    CHECK(reloaded.main_cfg["plugin_settings"]["a"] == 1);
    CHECK(reloaded.main_cfg["extra"]["k"] == 1);
}

TEST_CASE("SatDumpConfigHandler saveUser without changes round-trips to the defaults", "[core][config]")
{
    ConfigEnv env;
    {
        satdump::SatDumpConfigHandler cfg;
        cfg.load(env.defaults_file.string(), env.user.string());
        cfg.saveUser();
    }
    // An empty diff is stored as JSON null
    CHECK(readJson(env.user / "settings.json").is_null());

    satdump::SatDumpConfigHandler reloaded;
    reloaded.load(env.defaults_file.string(), env.user.string());
    CHECK(reloaded.main_cfg == defaults());
}

TEST_CASE("SatDumpConfigHandler loadUser with malformed settings throws", "[core][config]")
{
    ConfigEnv env;
    writeFile(env.user / "settings.json", "{ this is not json");
    satdump::SatDumpConfigHandler cfg;
    // The parse error propagates (initSatDump catches it and exits)
    CHECK_THROWS_AS(cfg.load(env.defaults_file.string(), env.user.string()), nlohmann::json::parse_error);
}

TEST_CASE("SatDumpConfigHandler typed getters read the value fields", "[core][config]")
{
    ConfigEnv env;
    satdump::SatDumpConfigHandler cfg;
    cfg.load(env.defaults_file.string(), env.user.string());

    CHECK(cfg.getValueFromSatDumpGeneral<int>("buffer") == 1024);
    CHECK(cfg.getValueFromSatDumpGeneral<std::string>("kepler_update_interval") == "1 day");
    CHECK(cfg.getValueFromSatDumpDirectories<std::string>("recording_path") == "/data/rec");
    CHECK(cfg.getValueFromSatDumpUI<std::string>("theme") == "Dark");
    CHECK(cfg.shouldAutoprocessProducts());
    CHECK_FALSE(cfg.shouldPlayAudio());

    CHECK_THROWS_AS(cfg.getValueFromSatDumpGeneral<int>("missing_key"), satdump::satdump_exception_t);
    CHECK_THROWS_AS(cfg.getValueFromSatDumpDirectories<std::string>("missing_key"), satdump::satdump_exception_t);
    CHECK_THROWS_AS(cfg.getValueFromSatDumpUI<bool>("missing_key"), satdump::satdump_exception_t);
    CHECK_THROWS_WITH(cfg.getValueFromSatDumpGeneral<int>("missing_key"), Catch::Matchers::StartsWith("Config key missing_key is missing!"));
    // A value of the wrong type is a JSON type error
    CHECK_THROWS_AS(cfg.getValueFromSatDumpGeneral<int>("kepler_update_interval"), nlohmann::json::type_error);
}

TEST_CASE("SatDumpConfigHandler tryAssignValueFromSatDumpGeneral keeps the fallback when missing", "[core][config]")
{
    ConfigEnv env;
    satdump::SatDumpConfigHandler cfg;
    cfg.load(env.defaults_file.string(), env.user.string());

    int v = -1;
    cfg.tryAssignValueFromSatDumpGeneral(v, "buffer");
    CHECK(v == 1024);

    int w = 77;
    cfg.tryAssignValueFromSatDumpGeneral(w, "missing_key");
    CHECK(w == 77);
}

TEST_CASE("SatDumpConfigHandler loads the repository satdump_cfg.json", "[core][config]")
{
    ConfigEnv env;
    satdump::SatDumpConfigHandler cfg;
    // The shipped file has comments, which loadJsonFile must accept
    cfg.load(std::string(SATDUMP_TEST_SOURCE_DIR) + "/satdump_cfg.json", env.user.string());

    CHECK(cfg.main_cfg == cfg.default_cfg);
    CHECK(cfg.main_cfg.contains("satdump_general"));
    CHECK(cfg.main_cfg.contains("satdump_directories"));
    CHECK(cfg.main_cfg.contains("user_interface"));
    CHECK(cfg.main_cfg["user_interface"].contains("default_offline_parameters"));
    CHECK_NOTHROW(cfg.getValueFromSatDumpGeneral<std::string>("kepler_update_interval"));
    CHECK_NOTHROW(cfg.getValueFromSatDumpUI<std::string>("theme"));
}

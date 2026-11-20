#include "handlers/product/product_handler.h"
#include "../../pipeline/pipeline_test_helpers.h"
#include "core/exception.h"
#include "utils/time.h"
#include <catch2/catch_test_macros.hpp>

using namespace pipeline_test_helpers;
using satdump::handlers::ProductHandler;

namespace
{
    class TestProductHandler : public ProductHandler
    {
    public:
        std::vector<nlohmann::json> configs_set;

        using ProductHandler::ProductHandler;

        void drawMenu() {}
        void drawContents(ImVec2) {}
        void setConfig(nlohmann::json p) { configs_set.push_back(p); }

        // Expose protected helpers
        std::string fileName() { return generateFileName(); }
        void applyDefault() { tryApplyDefaultPreset(); }
        void setInternalName(const std::string &n) { product_internal_name = n; }

    protected:
        void do_process() {}
    };

    std::shared_ptr<satdump::products::Product> makeProduct(const std::string &instrument, const std::string &type = "unit_test_type")
    {
        auto p = std::make_shared<satdump::products::Product>();
        p->instrument_name = instrument;
        p->type = type;
        return p;
    }

    // Runs with the current directory set to a temporary folder holding a "resources" tree,
    // which takes precedence over the installed resources.
    struct ResourcesEnv
    {
        TempDir dir;
        CwdGuard cwd;
        ResourcesEnv() : dir("handlers_prod"), cwd(dir.path) { std::filesystem::create_directories(dir.path / "resources" / "instrument_cfgs" / "common"); }

        void write(const std::string &rel, const std::string &content) { writeFile((dir.path / "resources" / rel).string(), content); }
    };
} // namespace

TEST_CASE("ProductHandler names the handler after the instrument without a configuration", "[handlers][product_handler]")
{
    ResourcesEnv env;
    auto prod = makeProduct("unit_instr_nocfg");

    TestProductHandler plain(prod);
    CHECK(plain.getName() == "unit_instr_nocfg");
    CHECK(plain.getID() == "product_handler");
    CHECK(plain.getInstrumentCfg().is_null());

    prod->set_product_source("SAT-1");
    prod->set_product_id("ch4");
    prod->set_product_timestamp(1700000000);

    TestProductHandler full(prod);
    CHECK(full.getName() == "SAT-1 unit_instr_nocfg " + satdump::timestamp_to_string(1700000000) + " (ch4)");

    // In dataset mode, the source and timestamp are implied by the dataset
    TestProductHandler ds(prod, true);
    CHECK(ds.getName() == "unit_instr_nocfg (ch4)");
}

TEST_CASE("ProductHandler loads the instrument configuration and merges common presets", "[handlers][product_handler]")
{
    ResourcesEnv env;
    env.write("instrument_cfgs/unit_instr.json", R"({
        "name": "Unit Instrument",
        "default": { "preset": "default" },
        "presets": [ { "name": "Zulu" }, { "name": "Alpha", "needs": "ch9" } ]
    })");
    env.write("instrument_cfgs/common/extra.json", R"({ "instruments": ["unit_instr", "other"], "presets": [ { "name": "Mike" } ] })");
    env.write("instrument_cfgs/common/unrelated.json", R"({ "instruments": ["other"], "presets": [ { "name": "Other only" } ] })");
    env.write("instrument_cfgs/common/broken.json", "{ not json");

    auto prod = makeProduct("unit_instr");
    prod->set_product_source("SAT-2");

    std::vector<std::string> filtered;
    TestProductHandler h(prod, false,
                         [&filtered](nlohmann::ordered_json &p)
                         {
                             filtered.push_back(p["name"]);
                             return !p.contains("needs");
                         });

    CHECK(h.getName() == "SAT-2 Unit Instrument");

    auto cfg = h.getInstrumentCfg();
    REQUIRE(cfg["presets"].size() == 3);
    // Sorted alphabetically by name
    CHECK(cfg["presets"][0]["name"] == "Alpha");
    CHECK(cfg["presets"][1]["name"] == "Mike");
    CHECK(cfg["presets"][2]["name"] == "Zulu");

    // Every preset went through the filter callback
    CHECK(filtered == std::vector<std::string>{"Alpha", "Mike", "Zulu"});

    h.applyDefault();
    REQUIRE(h.configs_set.size() == 1);
    CHECK(h.configs_set[0] == nlohmann::json{{"preset", "default"}});
}

TEST_CASE("ProductHandler::tryApplyDefaultPreset does nothing without a default", "[handlers][product_handler]")
{
    ResourcesEnv env;
    env.write("instrument_cfgs/unit_nodef.json", R"({ "name": "No Default" })");

    TestProductHandler h(makeProduct("unit_nodef"));
    CHECK(h.getName() == "No Default");
    h.applyDefault();
    CHECK(h.configs_set.empty());
}

TEST_CASE("ProductHandler survives an invalid instrument configuration", "[handlers][product_handler]")
{
    ResourcesEnv env;
    env.write("instrument_cfgs/unit_bad.json", "{ broken");

    TestProductHandler h(makeProduct("unit_bad"));
    // The name is only set from a successfully parsed configuration, or when there is none at all
    CHECK(h.getName() == "");
    CHECK(h.getInstrumentCfg().is_null());
}

TEST_CASE("ProductHandler::saveResult is not implemented by default", "[handlers][product_handler]")
{
    ResourcesEnv env;
    TestProductHandler h(makeProduct("unit_instr_nocfg"));
    CHECK_THROWS_AS(h.saveResult(env.dir.str()), satdump::satdump_exception_t);
}

TEST_CASE("ProductHandler::generateFileName expands the filename template", "[handlers][product_handler]")
{
    ResourcesEnv env;
    MainConfigGuard cfg;
    TempDir out("handlers_prod_out");
    satdump::satdump_cfg.main_cfg["satdump_directories"]["default_image_output_directory"]["value"] = out.str();

    auto prod = makeProduct("unit cam");
    prod->set_product_source("Sat X");
    prod->set_product_timestamp(1700000000); // 2023-11-14 22:13:20 UTC

    TestProductHandler h(prod);
    h.setInternalName("rgb 1-2/3");

    satdump::satdump_cfg.main_cfg["satdump_directories"]["image_filename_template"]["value"] = "$t|$y-$M-$d $h:$m:$s|$i|$I|$c|$C|$a|$o|$O";
    // '/' is replaced by '-' to keep the name in one folder
    CHECK(h.fileName() == "1700000000|2023-11-14 22:13:20|unit cam|UNIT_CAM|rgb 1-2-3|RGB_1-2-3|1-2-3|Sat X|SAT_X");

    satdump::satdump_cfg.main_cfg["satdump_directories"]["image_filename_template"]["value"] = "$o_$c";
    CHECK(h.fileName() == "Sat X_rgb 1-2-3");
}

TEST_CASE("ProductHandler::generateFileName avoids existing files and defaults", "[handlers][product_handler]")
{
    ResourcesEnv env;
    MainConfigGuard cfg;
    TempDir out("handlers_prod_out");
    satdump::satdump_cfg.main_cfg["satdump_directories"]["default_image_output_directory"]["value"] = out.str();
    satdump::satdump_cfg.main_cfg["satdump_directories"]["image_filename_template"]["value"] = "img_$o_$y";

    auto prod = makeProduct("unit_cam");
    TestProductHandler h(prod);

    // No source and no timestamp: "Unknown" and the epoch
    CHECK(h.fileName() == "img_Unknown_1970");

    writeFile(out.file("img_Unknown_1970"), "x");
    CHECK(h.fileName() == "img_Unknown_1970_2");
    writeFile(out.file("img_Unknown_1970_2"), "x");
    CHECK(h.fileName() == "img_Unknown_1970_3");
}

TEST_CASE("getProductHandlerForProduct dispatches through the event bus", "[handlers][product_handler]")
{
    ResourcesEnv env;
    EventBusGuard bus;

    CHECK_THROWS_AS(satdump::handlers::getProductHandlerForProduct(nullptr), satdump::satdump_exception_t);

    auto prod = makeProduct("unit_instr_nocfg", "unit_plugin_type");
    CHECK_THROWS_AS(satdump::handlers::getProductHandlerForProduct(prod), satdump::satdump_exception_t);

    // A plugin providing a handler for its own product type
    auto requests = std::make_shared<int>(0);
    satdump::eventBus->register_handler<satdump::handlers::RequestProductHandlerEvent>(
        [requests](satdump::handlers::RequestProductHandlerEvent evt)
        {
            (*requests)++;
            if (evt.product->type == "unit_plugin_type")
                evt.handler = std::make_shared<TestProductHandler>(evt.product, evt.dataset_mode);
        });

    auto h = satdump::handlers::getProductHandlerForProduct(prod, true);
    REQUIRE(h != nullptr);
    CHECK(*requests == 1);
    CHECK(h->getName() == "unit_instr_nocfg");
    CHECK(dynamic_cast<TestProductHandler *>(h.get()) != nullptr);

    auto other = makeProduct("unit_instr_nocfg", "unit_other_type");
    CHECK_THROWS_AS(satdump::handlers::getProductHandlerForProduct(other), satdump::satdump_exception_t);
}

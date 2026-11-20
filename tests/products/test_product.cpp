#include "products/dataset.h"
#include "products/image_product.h"
#include "products/product.h"
#include "products/punctiform_product.h"
#include "products_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <fstream>

using satdump::products::Product;
using satdump_test_products::ProductLoadersGuard;
using satdump_test_products::TempDir;

TEST_CASE("Product optional metadata accessors", "[products][product]")
{
    Product p;
    CHECK_FALSE(p.has_product_timestamp());
    CHECK_FALSE(p.has_product_source());
    CHECK_FALSE(p.has_product_id());

    p.set_product_timestamp(1700000000.25);
    p.set_product_source("MetOp-B");
    p.set_product_id("avhrr-1");
    CHECK(p.has_product_timestamp());
    CHECK(p.has_product_source());
    CHECK(p.has_product_id());
    CHECK(p.get_product_timestamp() == 1700000000.25);
    CHECK(p.get_product_source() == "MetOp-B");
    CHECK(p.get_product_id() == "avhrr-1");
}

TEST_CASE("Product save/load round-trips contents, instrument and type", "[products][product]")
{
    TempDir dir("product");
    {
        Product p;
        p.instrument_name = "test_instrument";
        p.type = "custom_type";
        p.set_product_timestamp(1234.5);
        p.set_product_source("TestSat");
        p.set_product_id("unit");
        p.contents["nested"] = {{"a", 1}, {"b", {1.5, 2.5}}, {"c", "text"}};
        p.save(dir.str());
    }
    REQUIRE(std::filesystem::exists(dir.path / "product.cbor"));

    Product q;
    q.load((dir.path / "product.cbor").string());
    CHECK(q.instrument_name == "test_instrument");
    CHECK(q.type == "custom_type");
    CHECK(q.get_product_timestamp() == 1234.5);
    CHECK(q.get_product_source() == "TestSat");
    CHECK(q.get_product_id() == "unit");
    CHECK(q.contents["nested"] == nlohmann::json{{"a", 1}, {"b", {1.5, 2.5}}, {"c", "text"}});
    CHECK(q.contents["instrument"] == "test_instrument");
    CHECK(q.contents["type"] == "custom_type");
}

TEST_CASE("Product load rejects missing or invalid files", "[products][product]")
{
    TempDir dir("product_bad");
    Product p;
    CHECK_THROWS(p.load((dir.path / "missing.cbor").string()));

    std::ofstream((dir.path / "garbage.cbor").string(), std::ios::binary) << "definitely not cbor";
    CHECK_THROWS(p.load((dir.path / "garbage.cbor").string()));

    // Valid CBOR, but no instrument/type
    auto cbor = nlohmann::json::to_cbor(nlohmann::json{{"foo", 1}});
    std::ofstream((dir.path / "incomplete.cbor").string(), std::ios::binary).write((char *)cbor.data(), cbor.size());
    CHECK_THROWS(p.load((dir.path / "incomplete.cbor").string()));
}

TEST_CASE("registerProducts registers the image and punctiform loaders", "[products][product]")
{
    ProductLoadersGuard guard;
    satdump::products::product_loaders.clear();
    satdump::products::product_loaders["stale"] = {};
    satdump::products::registerProducts();
    CHECK(satdump::products::product_loaders.count("image") == 1);
    CHECK(satdump::products::product_loaders.count("punctiform") == 1);
    CHECK(satdump::products::product_loaders.count("stale") == 0); // The registry is rebuilt from scratch
}

TEST_CASE("loadProduct picks the registered loader by product type", "[products][product]")
{
    ProductLoadersGuard guard;
    satdump::products::registerProducts();
    TempDir dir("loadproduct");

    SECTION("punctiform products load as PunctiformProduct, from the directory or the file")
    {
        satdump::products::PunctiformProduct p;
        p.instrument_name = "punct";
        p.data.push_back({"ch", {1.0}, {}, {42.0}});
        p.save(dir.str());

        for (auto path : {dir.str(), (dir.path / "product.cbor").string()})
        {
            auto loaded = satdump::products::loadProduct(path);
            REQUIRE(loaded);
            auto pp = std::dynamic_pointer_cast<satdump::products::PunctiformProduct>(loaded);
            REQUIRE(pp);
            CHECK(pp->instrument_name == "punct");
            REQUIRE(pp->data.size() == 1);
            CHECK(pp->data[0].data[0] == 42.0);
        }
    }

    SECTION("image products load as ImageProduct")
    {
        satdump_test_products::ConfigGuard cfg;
        cfg.setProductFormat("png");
        satdump::products::ImageProduct p;
        p.instrument_name = "img";
        p.save(dir.str());

        auto loaded = satdump::products::loadProduct(dir.str());
        REQUIRE(std::dynamic_pointer_cast<satdump::products::ImageProduct>(loaded));
        CHECK(loaded->type == "image");
    }

    SECTION("unknown types fall back to the base Product")
    {
        Product p;
        p.instrument_name = "other";
        p.type = "unregistered_type";
        p.save(dir.str());

        auto loaded = satdump::products::loadProduct(dir.str());
        REQUIRE(loaded);
        CHECK(loaded->type == "unregistered_type");
        CHECK(loaded->instrument_name == "other");
        CHECK_FALSE(std::dynamic_pointer_cast<satdump::products::ImageProduct>(loaded));
        CHECK_FALSE(std::dynamic_pointer_cast<satdump::products::PunctiformProduct>(loaded));
    }
}

TEST_CASE("DataSet save/load round-trips its fields", "[products][dataset]")
{
    TempDir dir("dataset");
    satdump::products::DataSet ds;
    ds.satellite_name = "NOAA-19";
    ds.timestamp = 1700000123.5;
    ds.products_list = {"AVHRR", "MHS", "sub/dir"};
    // save() takes a directory and writes dataset.json into it, load() takes the file itself
    ds.save(dir.str());
    REQUIRE(std::filesystem::exists(dir.path / "dataset.json"));

    satdump::products::DataSet ds2;
    ds2.load((dir.path / "dataset.json").string());
    CHECK(ds2.satellite_name == "NOAA-19");
    CHECK(ds2.timestamp == 1700000123.5);
    CHECK(ds2.products_list == std::vector<std::string>{"AVHRR", "MHS", "sub/dir"});
}

TEST_CASE("DataSet load rejects incomplete files", "[products][dataset]")
{
    TempDir dir("dataset_bad");
    std::ofstream((dir.path / "dataset.json").string()) << R"({"satellite": "X", "timestamp": 1})";
    satdump::products::DataSet ds;
    CHECK_THROWS(ds.load((dir.path / "dataset.json").string()));
}

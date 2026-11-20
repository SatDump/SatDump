#include "core/exception.h"
#include "core/params.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

// Only construction and value handling are tested; draw() needs an ImGui context.
// Note: the EditableParameter constructor calls rand() for its ImGui id.

using nlohmann::json;
using satdump::params::EditableParameter;

TEST_CASE("EditableParameter string and password values", "[core][params]")
{
    EditableParameter s(json{{"type", "string"}, {"name", "Name"}, {"value", "hello"}});
    CHECK(s.getValue() == "hello");
    s.setValue("changed");
    CHECK(s.getValue() == "changed");

    EditableParameter p(json{{"type", "password"}, {"name", "Secret"}, {"value", "hunter2"}});
    CHECK(p.getValue() == "hunter2");

    // Without a value both default to an empty string
    CHECK(EditableParameter(json{{"type", "string"}, {"name", "n"}}).getValue() == "");
    CHECK(EditableParameter(json{{"type", "password"}, {"name", "n"}}).getValue() == "");
}

TEST_CASE("EditableParameter numeric and bool values", "[core][params]")
{
    EditableParameter i(json{{"type", "int"}, {"name", "Count"}, {"value", -42}});
    CHECK(i.getValue() == -42);
    i.setValue(7);
    CHECK(i.getValue() == 7);
    CHECK(EditableParameter(json{{"type", "int"}, {"name", "n"}}).getValue() == 0);

    EditableParameter f(json{{"type", "float"}, {"name", "Gain"}, {"value", 12.5}});
    CHECK(f.getValue().get<double>() == Catch::Approx(12.5));
    f.setValue(-0.25);
    CHECK(f.getValue().get<double>() == Catch::Approx(-0.25));
    CHECK(EditableParameter(json{{"type", "float"}, {"name", "n"}}).getValue().get<double>() == 0.0);

    EditableParameter b(json{{"type", "bool"}, {"name", "Enabled"}, {"value", true}});
    CHECK(b.getValue() == true);
    b.setValue(false);
    CHECK(b.getValue() == false);
    CHECK(EditableParameter(json{{"type", "bool"}, {"name", "n"}}).getValue() == false);
}

TEST_CASE("EditableParameter wrong JSON value types throw", "[core][params]")
{
    CHECK_THROWS_AS(EditableParameter(json{{"type", "int"}, {"name", "n"}, {"value", "text"}}), json::type_error);
    EditableParameter i(json{{"type", "int"}, {"name", "n"}, {"value", 1}});
    CHECK_THROWS_AS(i.setValue("text"), json::type_error);
    // type and name are mandatory
    CHECK_THROWS_AS(EditableParameter(json{{"name", "n"}}), json::type_error);
    CHECK_THROWS_AS(EditableParameter(json{{"type", "int"}}), json::type_error);
}

TEST_CASE("EditableParameter options select by value", "[core][params]")
{
    json opts = {"low", "mid", "high"};
    EditableParameter o(json{{"type", "options"}, {"name", "Mode"}, {"options", opts}, {"value", "mid"}});
    CHECK(o.getValue() == "mid");
    o.setValue("high");
    CHECK(o.getValue() == "high");
    // Unknown values are ignored
    o.setValue("nonexistent");
    CHECK(o.getValue() == "high");

    // Without a value the first option is selected
    EditableParameter d(json{{"type", "options"}, {"name", "Mode"}, {"options", opts}});
    CHECK(d.getValue() == "low");
}

TEST_CASE("EditableParameter labeled_options use keys as values", "[core][params]")
{
    json opts = json::array({json::array({"k1", "Label One"}), json::array({"k2", "Label Two"})});

    EditableParameter l(json{{"type", "labeled_options"}, {"name", "Pick"}, {"options", opts}, {"value", "k2"}});
    CHECK(l.getValue() == "k2");
    l.setValue("k1");
    CHECK(l.getValue() == "k1");

    // Without a value the first key is used
    EditableParameter d(json{{"type", "labeled_options"}, {"name", "Pick"}, {"options", opts}});
    CHECK(d.getValue() == "k1");

    SECTION("manual entry allows arbitrary values")
    {
        EditableParameter m(json{{"type", "labeled_options"}, {"name", "Pick"}, {"options", opts}, {"manual", true}, {"value", "custom_value"}});
        CHECK(m.getValue() == "custom_value");
        m.setValue("another");
        CHECK(m.getValue() == "another");
        m.setValue("k2");
        CHECK(m.getValue() == "k2");
    }
}

TEST_CASE("EditableParameter color, baseband_type, path and notated_int values", "[core][params]")
{
    EditableParameter c(json{{"type", "color"}, {"name", "Color"}, {"value", {0.25, 0.5, 1.0}}});
    CHECK(c.getValue() == json({0.25, 0.5, 1.0}));
    c.setValue({1.0, 0.0, 0.5});
    CHECK(c.getValue() == json({1.0, 0.0, 0.5}));

    EditableParameter bb(json{{"type", "baseband_type"}, {"name", "Format"}, {"value", "cs16"}});
    CHECK(bb.getValue() == "cs16");
    bb.setValue("u8"); // Alias for cu8
    CHECK(bb.getValue() == "cu8");
    CHECK_THROWS_AS(bb.setValue("not_a_format"), satdump::satdump_exception_t);

    EditableParameter p(json{{"type", "path"}, {"name", "Input"}, {"value", "/tmp/some_file.bin"}, {"is_directory", false}});
    CHECK(p.getValue() == "/tmp/some_file.bin");
    p.setValue("/other");
    CHECK(p.getValue() == "/other");

    EditableParameter n(json{{"type", "notated_int"}, {"name", "Rate"}, {"value", 6000000}, {"units", "sps"}});
    CHECK(n.getValue() == 6000000);
    n.setValue(250000);
    CHECK(n.getValue() == 250000);
    CHECK(EditableParameter(json{{"type", "notated_int"}, {"name", "n"}}).getValue() == 0);
}

TEST_CASE("EditableParameter timestamp values", "[core][params]")
{
    EditableParameter t(json{{"type", "timestamp"}, {"name", "Start"}, {"value", 1700000000.0}});
    CHECK(t.getValue().get<double>() == Catch::Approx(1700000000.0));
    t.setValue(1600000000.0);
    CHECK(t.getValue().get<double>() == Catch::Approx(1600000000.0));

    // No value means "automatic", reported as -1
    EditableParameter a(json{{"type", "timestamp"}, {"name", "Start"}});
    CHECK(a.getValue().get<double>() == -1.0);
}

TEST_CASE("EditableParameter to_json writes name, description and value", "[core][params]")
{
    EditableParameter s(json{{"type", "string"}, {"name", "Name"}, {"description", "A description"}, {"value", "v"}});
    json j = s;
    CHECK(j["name"] == "Name");
    CHECK(j["description"] == "A description");
    CHECK(j["value"] == "v");

    EditableParameter nodesc(json{{"type", "int"}, {"name", "N"}, {"value", 3}});
    json j2 = nodesc;
    CHECK_FALSE(j2.contains("description"));
    CHECK(j2["value"] == 3);
}

// Bug: params.h to_json computes the type string ("string", "int", ...) but never stores it in the
// JSON (the j["type"] assignment is missing), so serialising a parameter and reading it back with
// from_json throws because "type" is absent.
TEST_CASE("EditableParameter to_json and from_json round-trip", "[core][params][!shouldfail]")
{
    EditableParameter s(json{{"type", "int"}, {"name", "Count"}, {"value", 11}});
    json j = s;
    CHECK(j["type"] == "int");
    EditableParameter back;
    CHECK_NOTHROW(back = j.get<EditableParameter>());
}

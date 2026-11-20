#include "utils/string.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <filesystem>
#include <fstream>

using namespace satdump;

TEST_CASE("splitString splits on the delimiter", "[utils][string]")
{
    using vec = std::vector<std::string>;

    CHECK(splitString("a,b,c", ',') == vec{"a", "b", "c"});
    CHECK(splitString("no delimiter", ',') == vec{"no delimiter"});
    CHECK(splitString("a,,b", ',') == vec{"a", "", "b"});
    CHECK(splitString(",a", ',') == vec{"", "a"});
    CHECK(splitString("", ',').empty());
}

TEST_CASE("replaceAllStr replaces every occurrence", "[utils][string]")
{
    std::string s = "one two one";
    replaceAllStr(s, "one", "1");
    CHECK(s == "1 two 1");

    SECTION("replacement containing the pattern does not loop")
    {
        std::string x = "xx";
        replaceAllStr(x, "x", "yx");
        CHECK(x == "yxyx");
    }

    SECTION("empty pattern is a no-op")
    {
        std::string x = "abc";
        replaceAllStr(x, "", "z");
        CHECK(x == "abc");
    }

    SECTION("missing pattern is a no-op")
    {
        std::string x = "abc";
        replaceAllStr(x, "d", "z");
        CHECK(x == "abc");
    }

    SECTION("replacing with nothing removes the pattern")
    {
        std::string x = "a-b-c";
        replaceAllStr(x, "-", "");
        CHECK(x == "abc");
    }
}

TEST_CASE("isStringPresent is case-insensitive", "[utils][string]")
{
    CHECK(isStringPresent("Hello World", "WORLD"));
    CHECK(isStringPresent("NOAA-19 HRPT", "hrpt"));
    CHECK(isStringPresent("anything", ""));
    CHECK_FALSE(isStringPresent("Hello World", "planet"));
    CHECK_FALSE(isStringPresent("", "x"));
}

TEST_CASE("loadFileToString reads a whole file", "[utils][string]")
{
    auto path = std::filesystem::temp_directory_path() / "satdump_tests_load_file.txt";
    const std::string content = "line one\nline two\n";
    {
        std::ofstream f(path, std::ios::binary);
        f << content;
    }

    CHECK(loadFileToString(path.string()) == content);
    std::filesystem::remove(path);

    CHECK(loadFileToString(path.string()).empty());
}

TEST_CASE("ws2s and s2ws round-trip UTF-8", "[utils][string]")
{
    CHECK(ws2s(L"plain") == "plain");
    CHECK(s2ws("plain") == L"plain");

    const std::string utf8 = "h\xC3\xA9llo"; // "héllo"
    std::wstring wide = s2ws(utf8);
    CHECK(wide.size() == 5);
    CHECK(wide[1] == L'é');
    CHECK(ws2s(wide) == utf8);
}

#include "core/exception.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <stdexcept>
#include <string>

namespace
{
    // The file part of what() is __FILE__ with the repository root (SOURCE_PATH_SIZE chars) cut off
    std::string thisFile() { return std::string(__FILE__).substr(SOURCE_PATH_SIZE); }

    [[noreturn]] void throwAt(int &line)
    {
        line = __LINE__ + 1;
        throw satdump_exception("something broke");
    }
} // namespace

TEST_CASE("satdump_exception what() appends the throwing source location", "[core][exception]")
{
    CHECK(thisFile() == "tests/core/test_exception.cpp");

    int line = 0;
    try
    {
        throwAt(line);
        FAIL("No exception thrown");
    }
    catch (satdump::satdump_exception_t &e)
    {
        CHECK(std::string(e.what()) == "something broke => " + thisFile() + ":" + std::to_string(line));
    }
}

TEST_CASE("satdump_exception is a std::runtime_error carrying the bare message", "[core][exception]")
{
    int line = 0;
    try
    {
        throwAt(line);
    }
    catch (std::runtime_error &e)
    {
        // Through the base class reference what() is still the decorated message...
        CHECK(std::string(e.what()).rfind("something broke => ", 0) == 0);
        // ...while the runtime_error part keeps the original text
        CHECK(std::string(e.std::runtime_error::what()) == "something broke");
    }
}

TEST_CASE("satdump_exception_t handles paths shorter than the source prefix", "[core][exception]")
{
    satdump::satdump_exception_t e("msg", "x.cpp", 7);
    CHECK(std::string(e.what()) == "msg => :7");

    satdump::satdump_exception_t empty("", "", 0);
    CHECK(std::string(empty.what()) == " => :0");
}

TEST_CASE("satdump_exception accepts std::string messages", "[core][exception]")
{
    std::string m = "built " + std::to_string(3);
    CHECK_THROWS_AS([&]() { throw satdump_exception(m); }(), satdump::satdump_exception_t);
    CHECK_THROWS_WITH([&]() { throw satdump::satdump_exception_t(m, "/a/b", 1); }(), "built 3 => :1");
}

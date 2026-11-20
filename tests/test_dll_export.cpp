#include "dll_export.h"
#include <catch2/catch_test_macros.hpp>
#include <string>

#define SATDUMP_TEST_STR2(x) #x
#define SATDUMP_TEST_STR(x) SATDUMP_TEST_STR2(x)

TEST_CASE("dll_export macros expand to nothing outside MSVC", "[dll_export]")
{
#ifndef _MSC_VER
    CHECK(std::string(SATDUMP_TEST_STR(SATDUMP_DLL)) == "");
    CHECK(std::string(SATDUMP_TEST_STR(SATDUMP_DLL2)) == "");
#else
    SUCCEED("MSVC uses __declspec");
#endif
}

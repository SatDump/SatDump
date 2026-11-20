#include "handlers/vector/dbf_file/dbf_file.h"
#include "../../../pipeline/pipeline_test_helpers.h"
#include "core/exception.h"
#include <catch2/catch_test_macros.hpp>
#include <cstring>

using namespace pipeline_test_helpers;

namespace
{
    struct Field
    {
        std::string name;
        char type;
        uint8_t length;
    };

    void put16(std::string &s, size_t pos, uint16_t v)
    {
        s[pos] = v & 0xFF;
        s[pos + 1] = v >> 8;
    }

    void put32(std::string &s, size_t pos, uint32_t v)
    {
        for (int i = 0; i < 4; i++)
            s[pos + i] = (v >> (8 * i)) & 0xFF;
    }

    // Builds a dBASE III file. Each row is {deleted, values...}; values are padded with spaces.
    std::string makeDbf(const std::vector<Field> &fields, const std::vector<std::pair<bool, std::vector<std::string>>> &rows, bool eof_marker = true, int header_records = -1)
    {
        uint16_t record_size = 1;
        for (auto &f : fields)
            record_size += f.length;

        std::string out(32, '\0');
        out[0] = 0x03;
        out[1] = 124, out[2] = 1, out[3] = 1;
        put32(out, 4, header_records >= 0 ? header_records : rows.size());
        put16(out, 8, 32 + 32 * fields.size() + 1);
        put16(out, 10, record_size);

        for (auto &f : fields)
        {
            std::string d(32, '\0');
            memcpy(&d[0], f.name.data(), std::min<size_t>(f.name.size(), 10));
            d[11] = f.type;
            d[16] = f.length;
            out += d;
        }
        out += '\x0D';

        for (auto &r : rows)
        {
            out += r.first ? '*' : ' ';
            for (size_t i = 0; i < fields.size(); i++)
            {
                std::string v = r.second[i];
                v.resize(fields[i].length, ' ');
                out += v;
            }
        }
        if (eof_marker)
            out += '\x1A';
        return out;
    }

    const std::vector<Field> CITY_FIELDS = {{"name", 'C', 12}, {"pop", 'N', 8}};
} // namespace

TEST_CASE("dbf_file::readDbfFile reads records as trimmed strings", "[handlers][dbf_file]")
{
    TempDir tmp("handlers_dbf");
    std::string path = tmp.file("cities.dbf");
    writeFile(path, makeDbf(CITY_FIELDS, {{false, {"Paris", "2148000"}}, {false, {"Lyon", "513000"}}, {false, {"Nice", "342000"}}}));

    auto j = dbf_file::readDbfFile(path);

    REQUIRE(j.is_array());
    REQUIRE(j.size() == 3);
    CHECK(j[0]["name"] == "Paris");
    CHECK(j[0]["pop"] == "2148000");
    CHECK(j[1]["name"] == "Lyon");
    CHECK(j[1]["pop"] == "513000");
    CHECK(j[2]["name"] == "Nice");
    CHECK(j[2]["pop"] == "342000");
    CHECK(j[0].size() == 2);
}

TEST_CASE("dbf_file::readDbfFile skips deleted records", "[handlers][dbf_file]")
{
    TempDir tmp("handlers_dbf");
    std::string path = tmp.file("cities.dbf");
    writeFile(path, makeDbf(CITY_FIELDS, {{false, {"Keep1", "1"}}, {true, {"Gone", "2"}}, {false, {"Keep2", "3"}}}));

    auto j = dbf_file::readDbfFile(path);
    REQUIRE(j.size() == 2);
    CHECK(j[0]["name"] == "Keep1");
    CHECK(j[1]["name"] == "Keep2");
    CHECK(j[1]["pop"] == "3");
}

TEST_CASE("dbf_file::readDbfFile handles a missing end-of-file marker", "[handlers][dbf_file]")
{
    TempDir tmp("handlers_dbf");
    std::string path = tmp.file("cities.dbf");
    writeFile(path, makeDbf(CITY_FIELDS, {{false, {"A", "1"}}, {false, {"B", "2"}}, {false, {"C", "3"}}}, false));

    auto j = dbf_file::readDbfFile(path);
    REQUIRE(j.size() == 3);
    CHECK(j[2]["name"] == "C");
}

TEST_CASE("dbf_file::readDbfFile only trims trailing spaces", "[handlers][dbf_file]")
{
    TempDir tmp("handlers_dbf");
    std::string path = tmp.file("cities.dbf");
    writeFile(path, makeDbf(CITY_FIELDS, {{false, {"  Lead", "     42"}}, {false, {"", ""}}, {false, {"x", "y"}}}));

    auto j = dbf_file::readDbfFile(path);
    REQUIRE(j.size() == 3);
    CHECK(j[0]["name"] == "  Lead");
    CHECK(j[0]["pop"] == "     42");
    // An all-blank field keeps a single space
    CHECK(j[1]["name"] == " ");
    CHECK(j[1]["pop"] == " ");
}

TEST_CASE("dbf_file::readDbfFile throws for a missing file", "[handlers][dbf_file]")
{
    TempDir tmp("handlers_dbf");
    CHECK_THROWS_AS(dbf_file::readDbfFile(tmp.file("nope.dbf")), satdump::satdump_exception_t);
}

// dbf_file.cpp:29 : the field descriptor loop is bounded by header.m_uNumRecords (the number of
// rows) instead of the number of fields, so files with fewer rows than columns lose fields and the
// remaining descriptors are then parsed as row data.
TEST_CASE("dbf_file::readDbfFile reads all fields when there are fewer rows than fields", "[handlers][dbf_file][!shouldfail]")
{
    TempDir tmp("handlers_dbf");
    std::string path = tmp.file("one.dbf");
    writeFile(path, makeDbf({{"a", 'C', 4}, {"b", 'C', 4}, {"c", 'C', 4}}, {{false, {"x", "y", "z"}}}));

    auto j = dbf_file::readDbfFile(path);
    REQUIRE(j.is_array());
    CHECK(j.size() == 1);
    CHECK(j[0]["a"] == "x");
    CHECK(j[0]["b"] == "y");
    CHECK(j[0]["c"] == "z");
}

TEST_CASE("dbf_file::readDbfFile reads the shipped populated places table", "[handlers][dbf_file]")
{
    std::string path = std::string(SATDUMP_TEST_SOURCE_DIR) + "/resources/maps/ne_10m_populated_places_simple.dbf";
    REQUIRE(std::filesystem::exists(path));

    // Number of records, read independently from the header
    std::string raw = readFile(path);
    REQUIRE(raw.size() > 32);
    uint32_t n_records = (uint8_t)raw[4] | (uint8_t)raw[5] << 8 | (uint8_t)raw[6] << 16 | (uint32_t)(uint8_t)raw[7] << 24;

    auto j = dbf_file::readDbfFile(path);
    REQUIRE(j.is_array());
    CHECK(j.size() == n_records);
    CHECK(j[0].contains("name"));
    CHECK(j[0]["name"].is_string());
    CHECK(j[0].contains("scalerank"));
}

#include "common/mpeg_ts/fazzt_processor.h"
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <vector>

namespace
{
    const int PAYLOAD = 100;

    void putLE16(std::vector<uint8_t> &f, int pos, uint16_t v)
    {
        f[pos] = v & 0xFF;
        f[pos + 1] = v >> 8;
    }

    void putLE32(std::vector<uint8_t> &f, int pos, uint32_t v)
    {
        for (int i = 0; i < 4; i++)
            f[pos + i] = v >> (8 * i);
    }

    std::vector<uint8_t> headFrame(uint32_t id, const std::string &name, uint16_t parts, uint32_t length)
    {
        std::vector<uint8_t> f(300, 0);
        f[1] = 0x03;
        putLE16(f, 2, f.size());
        putLE32(f, 4, id);
        putLE16(f, 72, parts);
        memcpy(&f[84], name.c_str(), name.size() + 1);
        putLE32(f, 84 + name.size() + 56, length);
        memcpy(&f[84 + name.size() + 72], "path", 5);
        return f;
    }

    std::vector<uint8_t> bodyFrame(uint32_t id, uint16_t part, const std::vector<uint8_t> &payload)
    {
        std::vector<uint8_t> f(16, 0);
        f.insert(f.end(), payload.begin(), payload.end());
        f[1] = 0x01;
        putLE16(f, 2, f.size());
        putLE32(f, 4, id);
        putLE16(f, 8, part);
        return f;
    }

    std::vector<uint8_t> tailFrame(uint32_t id)
    {
        std::vector<uint8_t> f(16, 0);
        f[1] = 0xFF;
        putLE16(f, 2, f.size());
        putLE32(f, 4, id);
        return f;
    }

    std::vector<uint8_t> partData(int part)
    {
        std::vector<uint8_t> d(PAYLOAD);
        for (int i = 0; i < PAYLOAD; i++)
            d[i] = part * 31 + i;
        return d;
    }
} // namespace

TEST_CASE("fazzt::FazztProcessor reassembles a file from head, body and tail", "[common][mpeg_ts][fazzt]")
{
    fazzt::FazztProcessor proc(PAYLOAD);
    const uint32_t id = 0xA1B2C3D4;

    CHECK(proc.work(headFrame(id, "image.png", 3, 250)).empty());
    // Parts may arrive out of order
    CHECK(proc.work(bodyFrame(id, 2, partData(2))).empty());
    CHECK(proc.work(bodyFrame(id, 0, partData(0))).empty());
    CHECK(proc.work(bodyFrame(id, 1, partData(1))).empty());
    auto files = proc.work(tailFrame(id));

    REQUIRE(files.size() == 1);
    const fazzt::FazztFile &file = files[0];
    CHECK(file.name == "image.png");
    CHECK(file.size == 250);
    CHECK(file.parts == 3);
    REQUIRE(file.data.size() == 250);

    std::vector<uint8_t> expected;
    for (int p = 0; p < 3; p++)
    {
        auto d = partData(p);
        expected.insert(expected.end(), d.begin(), d.end());
    }
    expected.resize(250);
    CHECK(file.data == expected);

    // The file is done; a second tail yields nothing
    CHECK(proc.work(tailFrame(id)).empty());
}

TEST_CASE("fazzt::FazztProcessor keeps concurrent files apart", "[common][mpeg_ts][fazzt]")
{
    fazzt::FazztProcessor proc(PAYLOAD);

    proc.work(headFrame(1, "first.bin", 1, 10));
    proc.work(headFrame(2, "second.bin", 1, 20));
    proc.work(bodyFrame(2, 0, partData(2)));
    proc.work(bodyFrame(1, 0, partData(1)));

    auto f2 = proc.work(tailFrame(2));
    REQUIRE(f2.size() == 1);
    CHECK(f2[0].name == "second.bin");
    auto d2 = partData(2);
    CHECK(f2[0].data == std::vector<uint8_t>(d2.begin(), d2.begin() + 20));

    auto f1 = proc.work(tailFrame(1));
    REQUIRE(f1.size() == 1);
    CHECK(f1[0].name == "first.bin");
    auto d1 = partData(1);
    CHECK(f1[0].data == std::vector<uint8_t>(d1.begin(), d1.begin() + 10));
}

TEST_CASE("fazzt::FazztProcessor ignores packets for unknown files", "[common][mpeg_ts][fazzt]")
{
    fazzt::FazztProcessor proc(PAYLOAD);
    CHECK(proc.work(bodyFrame(5, 0, partData(0))).empty());
    CHECK(proc.work(tailFrame(5)).empty());
    // Body before head is dropped, so a later head + tail gives a zero filled file
    proc.work(headFrame(5, "late.bin", 1, 4));
    auto files = proc.work(tailFrame(5));
    REQUIRE(files.size() == 1);
    CHECK(files[0].data == std::vector<uint8_t>(4, 0));
}

TEST_CASE("fazzt::FazztProcessor rejects invalid heads", "[common][mpeg_ts][fazzt]")
{
    fazzt::FazztProcessor proc(PAYLOAD);

    SECTION("filename of 4 characters or less")
    {
        proc.work(headFrame(1, "a.b", 1, 10));
        CHECK(proc.work(tailFrame(1)).empty());
    }

    SECTION("length larger than parts can hold")
    {
        proc.work(headFrame(1, "big.bin", 2, 201));
        CHECK(proc.work(tailFrame(1)).empty());
    }

    SECTION("length above MAX_SIZE")
    {
        proc.MAX_SIZE = 100;
        proc.work(headFrame(1, "big.bin", 2, 150));
        CHECK(proc.work(tailFrame(1)).empty());
    }

    SECTION("zero length file never completes")
    {
        proc.work(headFrame(1, "empty.bin", 1, 0));
        CHECK(proc.work(tailFrame(1)).empty());
    }

    SECTION("packet length field larger than the frame")
    {
        auto head = headFrame(1, "file.bin", 1, 10);
        putLE16(head, 2, head.size() + 1);
        proc.work(head);
        CHECK(proc.work(tailFrame(1)).empty());
    }

    SECTION("head frame shorter than 85 bytes")
    {
        auto head = headFrame(1, "file.bin", 1, 10);
        head.resize(84);
        putLE16(head, 2, head.size());
        proc.work(head);
        CHECK(proc.work(tailFrame(1)).empty());
    }
}

TEST_CASE("fazzt::FazztProcessor ignores frames shorter than the header", "[common][mpeg_ts][fazzt]")
{
    fazzt::FazztProcessor proc(PAYLOAD);
    CHECK(proc.work({}).empty());
    CHECK(proc.work(std::vector<uint8_t>(7, 0xFF)).empty());
}

TEST_CASE("fazzt::FazztProcessor drops files older than MAX_TIME", "[common][mpeg_ts][fazzt]")
{
    fazzt::FazztProcessor proc(PAYLOAD);
    proc.MAX_TIME = -1; // Everything counts as expired

    proc.work(headFrame(1, "stale.bin", 1, 10));
    proc.work(bodyFrame(1, 0, partData(0))); // Expiry check runs after this frame
    CHECK(proc.work(tailFrame(1)).empty());
}

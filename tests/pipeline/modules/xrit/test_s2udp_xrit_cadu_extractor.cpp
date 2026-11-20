#include "pipeline/modules/xrit/module_s2udp_xrit_cadu_extractor.h"
#include "../../pipeline_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <random>

using namespace pipeline_test_helpers;
namespace pl = satdump::pipeline;

namespace
{
    const int PID = 0x123;

    std::vector<uint8_t> tsPacket(int pid, bool pusi, uint8_t cc)
    {
        std::vector<uint8_t> p(188, 0xFF);
        p[0] = 0x47;
        p[1] = (pusi ? 0x40 : 0x00) | ((pid >> 8) & 0x1F);
        p[2] = pid & 0xFF;
        p[3] = 0x10 | (cc & 0x0F); // payload only
        return p;
    }

    // Splits one section payload over TS packets for `pid` (pointer field = 0 in the first one).
    // The payload is only emitted by the demuxer once the next PUSI packet arrives.
    void appendSection(std::vector<uint8_t> &ts, int pid, const std::vector<uint8_t> &payload, uint8_t &cc)
    {
        size_t pos = 0;
        bool first = true;
        while (pos < payload.size() || first)
        {
            auto p = tsPacket(pid, first, cc++);
            size_t offset = first ? 5 : 4;
            if (first)
                p[4] = 0; // pointer field
            size_t n = std::min(payload.size() - pos, (size_t)188 - offset);
            std::copy(payload.begin() + pos, payload.begin() + pos + n, p.begin() + offset);
            pos += n;
            first = false;
            ts.insert(ts.end(), p.begin(), p.end());
        }
    }

    // 40 bytes of MPE/IP/UDP headers, then the CADU
    std::vector<uint8_t> ipPayload(const std::vector<uint8_t> &cadu)
    {
        std::vector<uint8_t> p(40, 0x00);
        p.insert(p.end(), cadu.begin(), cadu.end());
        return p;
    }

    std::vector<uint8_t> makeCadu(uint32_t seed)
    {
        std::mt19937 rng(seed);
        std::vector<uint8_t> c(1024);
        for (auto &b : c)
            b = rng() & 0xFF;
        c[0] = 0x1A, c[1] = 0xCF, c[2] = 0xFC, c[3] = 0x1D;
        return c;
    }

    std::shared_ptr<pl::ProcessingModule> runExtractor(const std::string &input, const std::string &hint)
    {
        auto m = pl::xrit::S2UDPxRITCADUextractor::getInstance(input, hint, {{"pid", PID}, {"bb_size", 38688}, {"ts_input", true}});
        m->setInputType(pl::DATA_FILE);
        m->setOutputType(pl::DATA_FILE);
        m->init();
        m->process();
        return m;
    }
} // namespace

TEST_CASE("S2UDPxRITCADUextractor identifies itself", "[pipeline][modules][s2udp_xrit_cadu_extractor]")
{
    CHECK(pl::xrit::S2UDPxRITCADUextractor::getID() == "s2_udp_cadu_extractor");
    auto m = pl::xrit::S2UDPxRITCADUextractor::getInstance("in", "out", {{"pid", PID}, {"bb_size", 38688}});
    CHECK(m->getIDM() == "s2_udp_cadu_extractor");
    CHECK(m->getInputTypes() == std::vector<pl::ModuleDataType>{pl::DATA_FILE, pl::DATA_STREAM});
    CHECK(m->getOutputTypes() == std::vector<pl::ModuleDataType>{pl::DATA_FILE});
}

TEST_CASE("S2UDPxRITCADUextractor requires pid and bb_size", "[pipeline][modules][s2udp_xrit_cadu_extractor]")
{
    CHECK_THROWS(pl::xrit::S2UDPxRITCADUextractor::getInstance("in", "out", {{"bb_size", 38688}}));
    CHECK_THROWS(pl::xrit::S2UDPxRITCADUextractor::getInstance("in", "out", {{"pid", PID}}));
}

TEST_CASE("S2UDPxRITCADUextractor extracts CADUs from UDP payloads in a transport stream", "[pipeline][modules][s2udp_xrit_cadu_extractor]")
{
    TempDir tmp("pipe_s2udp");

    auto cadu1 = makeCadu(1);
    auto cadu2 = makeCadu(2);
    auto cadu_other_pid = makeCadu(3);
    std::vector<uint8_t> not_a_cadu(1100, 0x55); // other IP traffic, no ASM at offset 40

    std::vector<uint8_t> ts;
    uint8_t cc = 0, cc_other = 0;
    appendSection(ts, PID, ipPayload(cadu1), cc);
    appendSection(ts, PID + 1, ipPayload(cadu_other_pid), cc_other);
    appendSection(ts, PID, not_a_cadu, cc);
    appendSection(ts, PID, ipPayload(cadu2), cc);
    // Terminating PUSI packet flushes the last section, then a null packet
    auto end = tsPacket(PID, true, cc++);
    end[4] = 0;
    ts.insert(ts.end(), end.begin(), end.end());
    auto null_pkt = tsPacket(0x1FFF, false, 0);
    ts.insert(ts.end(), null_pkt.begin(), null_pkt.end());

    std::string input = tmp.file("in.ts");
    writeBytes(input, ts);

    auto m = runExtractor(input, tmp.file("out"));

    CHECK(m->getOutput() == tmp.file("out") + ".cadu");
    std::vector<uint8_t> expected = cadu1;
    expected.insert(expected.end(), cadu2.begin(), cadu2.end());
    CHECK(readBytes(m->getOutput()) == expected);
}

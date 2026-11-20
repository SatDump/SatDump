#include "common/codings/viterbi/depunc.h"
#include "common/codings/viterbi/cc_decoder.h"
#include "common/codings/viterbi/cc_encoder.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <memory>
#include <string>
#include <vector>

namespace
{
    // DVB-S (EN 300 421) puncturing patterns, X then Y for each bit of the period
    struct Pattern
    {
        std::string name;
        std::string x, y;
        int states;   // received symbols per period
        float scale;  // get_berscale()
        std::shared_ptr<viterbi::puncturing::GenericDepunc> (*make)();
    };

    std::vector<Pattern> patterns()
    {
        using namespace viterbi::puncturing;
        return {
            {"2/3", "10", "11", 3, 3.5f, [] { return std::shared_ptr<GenericDepunc>(std::make_shared<Depunc23>()); }},
            {"3/4", "101", "110", 4, 5.0f, [] { return std::shared_ptr<GenericDepunc>(std::make_shared<Depunc34>()); }},
            {"5/6", "10101", "11010", 6, 8.0f, [] { return std::shared_ptr<GenericDepunc>(std::make_shared<Depunc56>()); }},
            {"7/8", "1000101", "1111010", 8, 10.0f, [] { return std::shared_ptr<GenericDepunc>(std::make_shared<Depunc78>()); }},
        };
    }

    std::vector<uint8_t> randomBytes(int count, uint32_t seed, bool bitsOnly = false)
    {
        std::vector<uint8_t> v(count);
        uint32_t state = seed;
        for (auto &b : v)
        {
            state = state * 1664525u + 1013904223u;
            b = bitsOnly ? state >> 31 : state >> 24;
            if (!bitsOnly && b == 128)
                b = 129; // keep 128 for erasures only
        }
        return v;
    }

    // Puncture a mother-code symbol stream (X0 Y0 X1 Y1 ...)
    std::vector<uint8_t> puncture(const std::vector<uint8_t> &mother, const Pattern &p)
    {
        std::vector<uint8_t> out;
        int period = p.x.size();
        for (size_t b = 0; b < mother.size() / 2; b++)
        {
            if (p.x[b % period] == '1')
                out.push_back(mother[b * 2]);
            if (p.y[b % period] == '1')
                out.push_back(mother[b * 2 + 1]);
        }
        return out;
    }

    // Reference depuncturing, starting at the beginning of a period, inserting 128 erasures
    std::vector<uint8_t> referenceDepuncture(const std::vector<uint8_t> &rx, const Pattern &p)
    {
        std::vector<uint8_t> out;
        int period = p.x.size();
        size_t in = 0;
        for (int b = 0; in < rx.size(); b++)
        {
            out.push_back(p.x[b % period] == '1' ? rx[in++] : 128);
            out.push_back(p.y[b % period] == '1' ? rx[in++] : 128);
        }
        return out;
    }

    std::vector<uint8_t> depuncStatic(viterbi::puncturing::GenericDepunc &d, std::vector<uint8_t> in, int shift)
    {
        std::vector<uint8_t> out(in.size() * 2 + 4, 0xEE);
        int n = d.depunc_static(in.data(), out.data(), in.size(), shift);
        out.resize(n);
        return out;
    }
}

TEST_CASE("GenericDepunc implementations report their period and BER scale", "[common][codings][viterbi]")
{
    for (auto &p : patterns())
    {
        INFO("rate " << p.name);
        auto d = p.make();
        CHECK(d->get_numstates() == p.states);
        CHECK(d->get_berscale() == p.scale);
        // Sanity check of the pattern table itself
        int kept = 0;
        for (char c : p.x + p.y)
            kept += c == '1';
        CHECK(kept == p.states);
    }
}

TEST_CASE("GenericDepunc::depunc_static matches the DVB-S puncturing patterns", "[common][codings][viterbi]")
{
    for (auto &p : patterns())
    {
        INFO("rate " << p.name);
        auto d = p.make();
        std::vector<uint8_t> rx = randomBytes(p.states * 50, p.states);
        std::vector<uint8_t> out = depuncStatic(*d, rx, 0);
        CHECK(out == referenceDepuncture(rx, p));
        CHECK(out.size() == p.x.size() * 2 * 50);
    }
}

TEST_CASE("GenericDepunc::depunc_static shifts within and beyond the period", "[common][codings][viterbi]")
{
    for (auto &p : patterns())
    {
        auto d = p.make();
        std::vector<uint8_t> rx = randomBytes(p.states * 20, 100 + p.states);
        for (int s = 1; s < p.states; s++)
        {
            INFO("rate " << p.name << ", shift " << s);
            // Shift s means rx[0] is the s-th received symbol of a period: equivalent to dropping the
            // depunctured output of the first s received symbols of an aligned stream
            std::vector<uint8_t> aligned(s, 7);
            aligned.insert(aligned.end(), rx.begin(), rx.end());
            std::vector<uint8_t> full = depuncStatic(*d, aligned, 0);
            std::vector<uint8_t> head = depuncStatic(*d, std::vector<uint8_t>(aligned.begin(), aligned.begin() + s), 0);
            std::vector<uint8_t> expected(full.begin() + head.size(), full.end());
            CHECK(depuncStatic(*d, rx, s) == expected);
        }
        for (int s = p.states; s < p.states * 2; s++)
        {
            INFO("rate " << p.name << ", shift " << s);
            // The upper half of the shifts prepends one erasure to swap the X/Y alignment
            std::vector<uint8_t> expected = {128};
            std::vector<uint8_t> base = depuncStatic(*d, rx, s - p.states);
            expected.insert(expected.end(), base.begin(), base.end());
            CHECK(depuncStatic(*d, rx, s) == expected);
        }
    }
}

TEST_CASE("GenericDepunc::depunc_cont over chunks equals one static pass, with even outputs", "[common][codings][viterbi]")
{
    for (auto &p : patterns())
    {
        for (int shift = 0; shift < p.states * 2; shift++)
        {
            INFO("rate " << p.name << ", shift " << shift);
            std::vector<uint8_t> rx = randomBytes(1000, shift * 7 + p.states);
            auto ref = p.make();
            std::vector<uint8_t> expected = depuncStatic(*ref, rx, shift);

            auto d = p.make();
            d->set_shift(shift);
            std::vector<uint8_t> got;
            size_t pos = 0;
            int chunk = 0;
            const int chunks[] = {1, 2, 3, 5, 7, 11, 13, 17, 64, 97};
            while (pos < rx.size())
            {
                int len = std::min<int>(chunks[chunk++ % 10], rx.size() - pos);
                std::vector<uint8_t> out(len * 2 + 4);
                int n = d->depunc_cont(&rx[pos], out.data(), len);
                REQUIRE(n % 2 == 0);
                got.insert(got.end(), out.begin(), out.begin() + n);
                pos += len;
            }
            // A trailing odd symbol stays buffered for the next call
            expected.resize(expected.size() & ~size_t(1));
            CHECK(got == expected);
        }
    }
}

TEST_CASE("GenericDepunc depunctured DVB-S streams decode with the k=7 Viterbi decoder", "[common][codings][viterbi]")
{
    const int nbits = 4200; // multiple of all periods
    std::vector<uint8_t> bits = randomBytes(nbits, 3, true);
    viterbi::CCEncoder enc(nbits, 7, 2, {79, 109});
    std::vector<uint8_t> mother(nbits * 2);
    enc.work(bits.data(), mother.data());
    for (auto &s : mother)
        s = s ? 255 : 0;

    for (auto &p : patterns())
    {
        INFO("rate " << p.name);
        auto d = p.make();
        std::vector<uint8_t> rx = puncture(mother, p);
        std::vector<uint8_t> dep = depuncStatic(*d, rx, 0);
        REQUIRE(dep.size() == mother.size());
        dep.resize(dep.size() + 64, 128);

        viterbi::CCDecoder dec(nbits, 7, 2, {79, 109});
        std::vector<uint8_t> out(nbits);
        dec.work(dep.data(), out.data());
        CHECK(out == bits);
    }
}

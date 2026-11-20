#include "common/cpu_features.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("cpu_features::get_cpu_features reports a consistent feature hierarchy", "[common][cpu_features]")
{
    cpu_features::cpu_features_t f = cpu_features::get_cpu_features();

    // Each detected level implies all the levels below it
    if (f.CPU_X86_AVX512)
        CHECK(f.CPU_X86_AVX2);
    if (f.CPU_X86_AVX2)
        CHECK(f.CPU_X86_AVX);
    if (f.CPU_X86_AVX)
        CHECK(f.CPU_X86_SSE42);
    if (f.CPU_X86_SSE42)
        CHECK(f.CPU_X86_SSE41);
    if (f.CPU_X86_SSE41)
        CHECK(f.CPU_X86_SSE4A);
    if (f.CPU_X86_SSE4A)
        CHECK(f.CPU_X86_SSE3);
    if (f.CPU_X86_SSE3)
        CHECK(f.CPU_X86_SSE2);
    if (f.CPU_ARM_NEON7 || f.CPU_ARM_NEON8)
        CHECK(f.CPU_ARM_NEON);

#if defined(__x86_64__) || defined(__i386__)
    CHECK_FALSE(f.CPU_ARM_NEON);
#endif
#if defined(__aarch64__) || defined(__arm__)
    CHECK_FALSE(f.CPU_X86_SSE2);
#endif
}

TEST_CASE("cpu_features::get_cpu_features is deterministic", "[common][cpu_features]")
{
    cpu_features::cpu_features_t a = cpu_features::get_cpu_features();
    cpu_features::cpu_features_t b = cpu_features::get_cpu_features();

    CHECK(a.CPU_X86_SSE2 == b.CPU_X86_SSE2);
    CHECK(a.CPU_X86_AVX == b.CPU_X86_AVX);
    CHECK(a.CPU_X86_AVX2 == b.CPU_X86_AVX2);
    CHECK(a.CPU_X86_AVX512 == b.CPU_X86_AVX512);
    CHECK(a.CPU_ARM_NEON == b.CPU_ARM_NEON);
}

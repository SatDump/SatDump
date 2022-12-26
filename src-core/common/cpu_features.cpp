#include "cpu_features.h"
#include <volk/volk.h>
#include <string>

namespace cpu_features
{
    cpu_features_t get_cpu_features()
    {
        std::string machine_string(volk_get_machine());
        cpu_features_t f;

        // X86 SSE
        if (machine_string.find("sse2") != std::string::npos)
            f.CPU_X86_SSE2 = true;
        if (machine_string.find("sse3") != std::string::npos)
            f.CPU_X86_SSE3 = f.CPU_X86_SSE2 = true;
        if (machine_string.find("sse4_a") != std::string::npos)
            f.CPU_X86_SSE4A = f.CPU_X86_SSE3 = f.CPU_X86_SSE2 = true;
        if (machine_string.find("sse4_1") != std::string::npos)
            f.CPU_X86_SSE41 = f.CPU_X86_SSE4A = f.CPU_X86_SSE3 = f.CPU_X86_SSE2 = true;
        if (machine_string.find("sse4_2") != std::string::npos)
            f.CPU_X86_SSE42 = f.CPU_X86_SSE41 = f.CPU_X86_SSE4A = f.CPU_X86_SSE3 = f.CPU_X86_SSE2 = true;

        // X86 AVX
        if (machine_string.find("avx") != std::string::npos)
            f.CPU_X86_AVX = f.CPU_X86_SSE42 = f.CPU_X86_SSE41 = f.CPU_X86_SSE4A = f.CPU_X86_SSE3 = f.CPU_X86_SSE2 = true;
        if (machine_string.find("avx2") != std::string::npos)
            f.CPU_X86_AVX2 = f.CPU_X86_AVX = f.CPU_X86_SSE42 = f.CPU_X86_SSE41 = f.CPU_X86_SSE4A = f.CPU_X86_SSE3 = f.CPU_X86_SSE2 = true;

        // ARM NEON
        if (machine_string.find("neon") != std::string::npos)
            f.CPU_ARM_NEON = true;
        if (machine_string.find("neonv7") != std::string::npos)
            f.CPU_ARM_NEON7 = f.CPU_ARM_NEON = true;
        if (machine_string.find("neonv8") != std::string::npos)
            f.CPU_ARM_NEON8 = f.CPU_ARM_NEON = true;

        return f;
    }

    void print_features(cpu_features_t f)
    {
        printf("Found CPU Features :\n");

        if (f.CPU_X86_SSE2)
            printf("- SSE2\n");
        if (f.CPU_X86_SSE3)
            printf("- SSE3\n");
        if (f.CPU_X86_SSE4A)
            printf("- SSE4_A\n");
        if (f.CPU_X86_SSE41)
            printf("- SSE4_1\n");
        if (f.CPU_X86_SSE42)
            printf("- SSE4_2\n");

        if (f.CPU_X86_AVX)
            printf("- AVX\n");
        if (f.CPU_X86_AVX2)
            printf("- AVX2\n");

        if (f.CPU_ARM_NEON)
            printf("- NEON\n");
        if (f.CPU_ARM_NEON7)
            printf("- NEONv7\n");
        if (f.CPU_ARM_NEON8)
            printf("- NEONv8\n");
    }
}
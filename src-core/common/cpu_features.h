#pragma once

namespace cpu_features
{
    struct cpu_features_t
    {
        bool CPU_X86_SSE2 = false;
        bool CPU_X86_SSE3 = false;
        bool CPU_X86_SSE4A = false;
        bool CPU_X86_SSE41 = false;
        bool CPU_X86_SSE42 = false;
        bool CPU_X86_AVX = false;
        bool CPU_X86_AVX2 = false;
        bool CPU_ARM_NEON = false;
        bool CPU_ARM_NEON7 = false;
        bool CPU_ARM_NEON8 = false;
    };

    /*
    Parse the Volk machine string to
    get available CPU features to utilize
    on this machine.
    */
    cpu_features_t get_cpu_features();
    void print_features(cpu_features_t f);
}
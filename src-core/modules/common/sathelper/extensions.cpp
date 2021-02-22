/*
 * extensions.cpp
 *
 *  Created on: 16/09/2017
 *      Author: Lucas Teske
 */

#include "extensions.h"

#ifdef MEMORY_OP_X86
//#include <immintrin.h>
#ifdef _MSC_VER
#include <intrin.h>
#endif

#define FMA3 (1 << 12)
#define SSE (1 << 26)
#define SSE4 (1 << 20)
#define XCR (1 << 27)
#define AVX (1 << 28)

#ifndef _MSC_VER
unsigned long long _xgetbv(unsigned int index)
{
    unsigned int eax, edx;
    __asm__ __volatile__(
        "xgetbv;"
        : "=a"(eax), "=d"(edx)
        : "c"(index));
    return ((unsigned long long)edx << 32) | eax;
}
#endif

void InitExtensions()
{
    unsigned int eax, ebx, ecx, edx;
    eax = 0x01;
#ifdef _MSC_VER
    int cpuInfo[4] = {-1};
    __cpuid(cpuInfo, 1);
    eax = cpuInfo[0];
    ebx = cpuInfo[1];
    ecx = cpuInfo[2];
    edx = cpuInfo[3];
#else
    __asm__ __volatile__(
        "cpuid;"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(eax));
#endif
    sathelper::Extensions::hasFMA = ecx & FMA3 || false;
    sathelper::Extensions::hasSSE = edx & SSE || false;
    sathelper::Extensions::hasSSE4 = ecx & SSE4 || false;

    sathelper::Extensions::hasAVX = ecx & AVX || false;
    bool osxsaveSupported = ecx & XCR || false;
    if (osxsaveSupported && sathelper::Extensions::hasAVX)
    {
        unsigned long long xcrFeatureMask = _xgetbv(0);
        sathelper::Extensions::hasAVX = (xcrFeatureMask & 0x6) == 0x6;
    }

#if 0
    std::cerr << "Has FMA: "  << (sathelper::Extensions::hasFMA  ? "true" : "false") << std::endl;
    std::cerr << "Has SSE: "  << (sathelper::Extensions::hasSSE  ? "true" : "false") << std::endl;
    std::cerr << "Has SSE4: " << (sathelper::Extensions::hasSSE4 ? "true" : "false") << std::endl;
    std::cerr << "Has AVX: "  << (sathelper::Extensions::hasAVX  ? "true" : "false") << std::endl;
#endif
}

#else
void InitExtensions()
{
    //std::cerr << "Non x86 device detected. No extensions supported." << std::endl;
}

#endif

namespace sathelper
{

    bool Extensions::hasFMA = false;
    bool Extensions::hasSSE = false;
    bool Extensions::hasSSE4 = false;
    bool Extensions::hasAVX = false;
    bool Extensions::initialized = (InitExtensions(), true);

} // namespace sathelper

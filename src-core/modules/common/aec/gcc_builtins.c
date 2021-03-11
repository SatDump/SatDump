/*
    MSVC lacks some builtin functions GCC has
*/
#ifdef _MSC_VER
#include <stdint.h>

uint32_t __builtin_clz(const uint64_t x)
{
    uint32_t u32 = (x >> 32);
    uint32_t result = u32 ? __builtin_clz(u32) : 32;
    if (result == 32)
    {
        u32 = x & 0xFFFFFFFFUL;
        result += (u32 ? __builtin_clz(u32) : 32);
    }
    return result;
}

uint32_t __builtin_clzll(uint64_t value)
{
    if (value == 0)
        return 64;
    uint32_t msh = (uint32_t)(value >> 32);
    uint32_t lsh = (uint32_t)(value & 0xFFFFFFFF);
    if (msh != 0)
        return __builtin_clz(msh);
    return 32 + __builtin_clz(lsh);
}
#endif
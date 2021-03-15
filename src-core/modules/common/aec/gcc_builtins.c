/*
    MSVC lacks some builtin functions GCC has
*/
#ifdef _MSC_VER
#include <intrin.h>
#include <stdint.h>

int __builtin_ctz(unsigned x) {
    unsigned long ret;
    _BitScanForward(&ret, x);
    return (int)ret;
}

int __builtin_ctzll(unsigned long long x) {
    unsigned long ret;
    _BitScanForward64(&ret, x);
    return (int)ret;
}

int __builtin_ctzl(unsigned long x) {
    return sizeof(x) == 8 ? __builtin_ctzll(x) : __builtin_ctz((uint32_t)x);
}

int __builtin_clz(unsigned x) {
    //unsigned long ret;
    //_BitScanReverse(&ret, x);
    //return (int)(31 ^ ret);
    return (int)__lzcnt(x);
}

int __builtin_clzll(unsigned long long x) {
    //unsigned long ret;
    //_BitScanReverse64(&ret, x);
    //return (int)(63 ^ ret);
    return (int)__lzcnt64(x);
}

int __builtin_clzl(unsigned long x) {
    return sizeof(x) == 8 ? __builtin_clzll(x) : __builtin_clz((uint32_t)x);
}

#ifdef __cplusplus
int __builtin_ctzl(unsigned long long x) {
    return __builtin_ctzll(x);
}

int __builtin_clzl(unsigned long long x) {
    return __builtin_clzll(x);
}
#endif
#endif
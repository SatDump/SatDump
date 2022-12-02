#define HAVE_MALLOC_H 1
#define HAVE_STDINT_H 1
/* #undef WORDS_BIGENDIAN */
#if defined(_WIN32)
#define HAVE_BSR64 1
#else
#define HAVE_DECL___BUILTIN_CLZLL 1
#endif

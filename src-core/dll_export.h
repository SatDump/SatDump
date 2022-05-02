#pragma once

#ifdef _MSC_VER
#ifdef SATDUMP_DLL_EXPORT
#define SATDUMP_DLL __declspec(dllexport)
#else
#define SATDUMP_DLL __declspec(dllimport)
#endif
#else
#define SATDUMP_DLL
#endif

#ifdef _MSC_VER
#ifdef SATDUMP_DLL_EXPORT2
#define SATDUMP_DLL2 __declspec(dllexport)
#else
#define SATDUMP_DLL2 __declspec(dllimport)
#endif
#else
#define SATDUMP_DLL2
#endif
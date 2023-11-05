/*
 * Copyright 2018-2022 Yury Gribov
 *
 * The MIT License (MIT)
 *
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE.txt file.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE // For RTLD_DEFAULT
#endif

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

// Sanity check for ARM to avoid puzzling runtime crashes
#ifdef __arm__
# if defined __thumb__ && ! defined __THUMB_INTERWORK__
#   error "ARM trampolines need -mthumb-interwork to work in Thumb mode"
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define CHECK(cond, fmt, ...) do { \
    if(!(cond)) { \
      fprintf(stderr, "implib-gen: AARONIA_RTSA_SUITE_LIB: " fmt "\n", ##__VA_ARGS__); \
      assert(0 && "Assertion in generated code"); \
      abort(); \
    } \
  } while(0)

#define HAS_DLOPEN_CALLBACK 1
#define HAS_DLSYM_CALLBACK 0
#define NO_DLOPEN 0
#define LAZY_LOAD 1

static void *lib_handle;
static int do_dlclose;
static int is_lib_loading;

#if ! NO_DLOPEN
static void *load_library() {
  if(lib_handle)
    return lib_handle;

  is_lib_loading = 1;

#if HAS_DLOPEN_CALLBACK
  extern void *mycallback(const char *lib_name);
  lib_handle = mycallback("AARONIA_RTSA_SUITE_LIB");
  CHECK(lib_handle, "failed to load library 'AARONIA_RTSA_SUITE_LIB' via callback 'mycallback'");
#else
  lib_handle = dlopen("AARONIA_RTSA_SUITE_LIB", RTLD_LAZY | RTLD_GLOBAL);
  CHECK(lib_handle, "failed to load library 'AARONIA_RTSA_SUITE_LIB' via dlopen: %s", dlerror());
#endif

  do_dlclose = 1;
  is_lib_loading = 0;

  return lib_handle;
}

static void __attribute__((destructor)) unload_lib() {
  if(do_dlclose && lib_handle)
    dlclose(lib_handle);
}
#endif

#if ! NO_DLOPEN && ! LAZY_LOAD
static void __attribute__((constructor)) load_lib() {
  load_library();
}
#endif

// TODO: convert to single 0-separated string
static const char *const sym_names[] = {
  "AARTSAAPI_Open",
  "AARTSAAPI_RescanDevices",
  "AARTSAAPI_EnumDevice",
  "AARTSAAPI_Close",
  "AARTSAAPI_ConfigFind",
  "AARTSAAPI_ConfigSetFloat",
  "AARTSAAPI_Close",
  "AARTSAAPI_CloseDevice",
  "AARTSAAPI_DisconnectDevice",
  "AARTSAAPI_StopDevice",
  "AARTSAAPI_GetPacket",
  "AARTSAAPI_StartDevice",
  "AARTSAAPI_ConnectDevice",
  "AARTSAAPI_ConfigSetString",
  "AARTSAAPI_ConfigFind",
  "AARTSAAPI_ConfigRoot",
  "AARTSAAPI_OpenDevice",
  0
};

#define SYM_COUNT (sizeof(sym_names)/sizeof(sym_names[0]) - 1)

extern void *_libAaroniaRTSAAPI_so_tramp_table[];

// Can be sped up by manually parsing library symtab...
void _libAaroniaRTSAAPI_so_tramp_resolve(int i) {
  assert((unsigned)i < SYM_COUNT);

  CHECK(!is_lib_loading, "library function '%s' called during library load", sym_names[i]);

  void *h = 0;
#if NO_DLOPEN
  // Library with implementations must have already been loaded.
  if (lib_handle) {
    // User has specified loaded library
    h = lib_handle;
  } else {
    // User hasn't provided us the loaded library so search the global namespace.
#   ifndef IMPLIB_EXPORT_SHIMS
    // If shim symbols are hidden we should search
    // for first available definition of symbol in library list
    h = RTLD_DEFAULT;
#   else
    // Otherwise look for next available definition
    h = RTLD_NEXT;
#   endif
  }
#else
  h = load_library();
  CHECK(h, "failed to resolve symbol '%s', library failed to load", sym_names[i]);
#endif

#if HAS_DLSYM_CALLBACK
  extern void *(void *handle, const char *sym_name);
  _libAaroniaRTSAAPI_so_tramp_table[i] = (h, sym_names[i]);
  CHECK(_libAaroniaRTSAAPI_so_tramp_table[i], "failed to resolve symbol '%s' via callback ", sym_names[i]);
#else
  // Dlsym is thread-safe so don't need to protect it.
  _libAaroniaRTSAAPI_so_tramp_table[i] = dlsym(h, sym_names[i]);
  CHECK(_libAaroniaRTSAAPI_so_tramp_table[i], "failed to resolve symbol '%s' via dlsym: %s", sym_names[i], dlerror());
#endif
}

// Helper for user to resolve all symbols
void _libAaroniaRTSAAPI_so_tramp_resolve_all(void) {
  size_t i;
  for(i = 0; i < SYM_COUNT; ++i)
    _libAaroniaRTSAAPI_so_tramp_resolve(i);
}

// Allows user to specify manually loaded implementation library.
void _libAaroniaRTSAAPI_so_tramp_set_handle(void *handle) {
  lib_handle = handle;
  do_dlclose = 0;
}

// Resets all resolved symbols. This is needed in case
// client code wants to reload interposed library multiple times.
void _libAaroniaRTSAAPI_so_tramp_reset(void) {
  memset(_libAaroniaRTSAAPI_so_tramp_table, 0, SYM_COUNT * sizeof(_libAaroniaRTSAAPI_so_tramp_table[0]));
  lib_handle = 0;
  do_dlclose = 0;
}

#ifdef __cplusplus
}  // extern "C"
#endif

#pragma once

#if ENABLE_I18N
#if defined(_WIN32) || defined(__ANDROID__)
#include "libs/libintl-tiny/libintl.h"
#else
#include <libintl.h>
#endif
#endif

#if ENABLE_I18N
#define _(STRING) gettext(STRING)
#else
#define _(STRING) (STRING)
#endif
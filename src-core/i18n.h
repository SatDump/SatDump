#pragma once

#if ENABLE_I18N
#include "i18n/libintl.h"
#endif

#if ENABLE_I18N
#define _(STRING) gettext(STRING)
#else
#define _(STRING) (STRING)
#endif
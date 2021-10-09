#pragma once

// Windows doesn't have strptime, damn it...
#ifdef _WIN32
#include <ctime>

char *strptime(const char *buf, const char *fmt, struct tm *tm);
#endif

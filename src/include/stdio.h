#ifndef OS_STDIO_H
#define OS_STDIO_H

#include <stdarg.h>

int vsprintf(char* buf, const char* fmt, va_list args);

#endif
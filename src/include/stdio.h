#ifndef OS_STDIO_H
#define OS_STDIO_H

#include <stdarg.h>

enum fd_t {
    stdin,
    stdout,
    stderr,
};

int vsprintf(char* buf, const char* fmt, va_list args);

int sprintf(char* buf, const char* fmt, ...);

int printf(const char* fmt, ...);

#endif
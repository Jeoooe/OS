#include <stdio.h>
#include <syscall.h>

static char buf[1024];

int printf(const char* fmt, ...) {
    va_list args;
    int i;
    va_start(args, fmt);
    i = vsprintf(buf, fmt, args);
    i = write(stdout, buf, i);
    va_end(args);
    return i;
}
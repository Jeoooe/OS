#include <stdio.h>
#include <syscall.h>

static char __buf[1024];

int printf(const char* fmt, ...) {
    va_list args;
    int i;
    va_start(args, fmt);
    i = vsprintf(__buf, fmt, args);
    i = write(stdout, __buf, i);
    va_end(args);
    return i;
}
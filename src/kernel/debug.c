#include <debug.h>
#include <os.h>
#include <stdarg.h>
#include <stdio.h>

static char buf[1024];

void debugk(char *file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    printk("[%s] [%d] %s\n", file, line, buf);
}
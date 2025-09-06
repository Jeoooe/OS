#include <os.h>
#include <stdarg.h>
#include <console.h>

static char buf[1024];

extern int vsprintf(char* buf, const char* fmt, va_list args);

int printk(const char *fmt, ...) {
    va_list args;
    int i;
    va_start(args, fmt);
    i = vsprintf(buf, fmt, args);
    console_write(buf, i);
    va_end(args);
    return i;
}
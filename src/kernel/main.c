#include <stdint.h>
#include <stdio.h>
#include <debug.h>
#include <console.h>

const char buf[] = "Kernel Init...";

extern void console_init();

void kernel_main() {
    console_init();
    console_write(buf, (size_t)sizeof(buf));
    while (1)
        ;
}
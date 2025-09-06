#include <os.h>
#include <stdint.h>
#include <stdio.h>
#include <debug.h>
#include <console.h>

// const char buf[] = "Kernel Init...";

extern void console_init();

void kernel_main() {
    console_init();
    printk("Kernel Init...\nYB %u", 799);
    while (1)
        ;
}
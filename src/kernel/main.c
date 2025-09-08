#include <os.h>
#include <stdint.h>
#include <stdio.h>
#include <debug.h>
#include <console.h>
#include <assert.h>

const char buf[] = "Kernel Init...";

extern void console_init();
extern void interrupt_init();

void kernel_main() {
    console_init();
    interrupt_init();
    printk("Kernel Init...\nYB is %u", 799);
    asm volatile("sti");
    BMB;
    while (1)
        ;
}
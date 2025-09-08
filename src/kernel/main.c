#include <os.h>
#include <stdint.h>
#include <stdio.h>
#include <debug.h>
#include <console.h>
#include <assert.h>
#include <interrupt.h>

const char buf[] = "Kernel Init...";

extern void console_init();
extern void interrupt_init();
extern void timer_init();

void kernel_main() {
    console_init();
    interrupt_init();
    timer_init();
    printk("Kernel Init...\nYB is %u\n", 799);
    bool flag = get_interrupt_state();
    set_interrupt_state(true);
    BMB;
    while (1)
        ;
}
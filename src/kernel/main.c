#include <os.h>
#include <stdint.h>
#include <stdio.h>
#include <debug.h>
#include <console.h>
#include <assert.h>
#include <interrupt.h>
#include <memory.h>

extern void interrupt_init();
extern void timer_init();

void kernel_main() {
    interrupt_init();
    timer_init();
    printk("Kernel Init...\nYB is %u\n", 799);
    void *addr = get_kpages(3);
    printk("Allocate 3 pages\nStart Addr: 0x%x\n", (uint32_t)addr);
    BMB;
    while (1)
        ;
}
#include <os.h>
#include <stdint.h>
#include <stdio.h>
#include <debug.h>
#include <console.h>
#include <assert.h>
#include <interrupt.h>
#include <memory.h>
#include <thread.h>

extern void interrupt_init();
extern void timer_init();

void tmp(void* arg) {
    char* para = (char*) arg;
    printk(para);
    while (1);
}

void kernel_main() {
    printk("Kernel Init...\nYB is %u\n", MAGIC);
    interrupt_init();
    timer_init();

    thread_start("k_thread_1", 31, tmp, "ARG1");

    while (1)
        ;
}
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
extern void task_init();

void tmp1(void* arg) {
    char* para = (char*) arg;
    while (1) {
        printk(para);
    }
}

void tmp2(void* arg) {
    char* para = (char*) arg;
    while (1) {
        printk(para);
    }
}

void kernel_main() {
    printk("Kernel Init...\nYB is %u\n", MAGIC);
    interrupt_init();
    timer_init();
    task_init();

    thread_create("k_thread_1", 31, tmp1, "arg1");
    thread_create("k_thread_2", 8, tmp2, "arg2");

    set_interrupt_state(true);

    while (1)
        ;
}
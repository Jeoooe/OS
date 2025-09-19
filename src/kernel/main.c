#include <os.h>
#include <stdint.h>
#include <stdio.h>
#include <debug.h>
#include <console.h>
#include <assert.h>
#include <interrupt.h>
#include <memory.h>
#include <thread.h>
#include <userprog.h>
#include <syscall.h>
#include <stdlib.h>

extern void interrupt_init();
extern void timer_init();
extern void task_init();
extern void keyboard_init();
extern void syscall_init();
extern void ide_init();

extern void* sys_malloc(uint32_t size);
extern void sys_free(void* ptr);

void k_th1() {
    void *a1 = sys_malloc(256);
    void *a2 = sys_malloc(256);
    void *a3 = sys_malloc(256);
    printk("thread 1 malloc :%p, %p, %p\n", a1, a2, a3);
    int delay = 1000000;
    while (delay-->0);
    sys_free(a1);
    sys_free(a2);
    sys_free(a3);
    while (1);
}

void k_th2() {
    void *a1 = sys_malloc(256);
    void *a2 = sys_malloc(256);
    void *a3 = sys_malloc(256);
    printk("thread 2 malloc :%p, %p, %p\n", a1, a2, a3);
    int delay = 1000000;
    while (delay-->0);
    sys_free(a1);
    sys_free(a2);
    sys_free(a3);
    while (1);
}

void u_th1() {
    void *a1 = malloc(256);
    void *a2 = malloc(256);
    void *a3 = malloc(256);
    printf("user 1 malloc :%p, %p, %p\n", a1, a2, a3);
    int delay = 100000;
    while (delay-->0);
    free(a1);
    free(a2);
    free(a3);
    while (1);
}

void kernel_main() {
    LOGK("Kernel Init...", MAGIC);
    interrupt_init();

    interrupt_mask(INTERRUPT_TIMER, true);
    interrupt_mask(INTERRUPT_SLAVE, true);
    interrupt_mask(INTERRUPT_HARDDISK_MASTER, true);
    interrupt_mask(INTERRUPT_HARDDISK_SLAVE, true);

    timer_init();
    task_init();
    syscall_init();
    ide_init();

    keyboard_init();
    
    // task_create("k_thread_2", 8, tmp2, "HELLOO");
    
    // task_create("k1", 31, k_th1, 0);
    // task_create("k2", 31, k_th2, 0);
    // process_execute(u_th1, "u1");
    // process_execute(u_th2, "u2");

    // interrupt_mask(INTERRUPT_KEYBOARD, true);
    
    set_interrupt_state(true);

    while (1)
        ;
}
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

extern void interrupt_init();
extern void timer_init();
extern void task_init();
extern void keyboard_init();
extern void syscall_init();

int ar1 = 0, ar2 = 0;
void u_th1() {
    ar1 = getpid();
    while (1) {
        printf("%d", ar1);
    }
}

void u_th2() {
    ar2 = getpid();
    while (1) {
        printf("%d", ar2);
    }
}
void k_th1() {
    while(1) {
        printk("%d", getpid());
    }
}

void kernel_main() {
    LOGK("Kernel Init...", MAGIC);
    interrupt_init();
    timer_init();
    task_init();

    syscall_init();
    keyboard_init();
    // task_create("k_thread_2", 8, tmp2, "HELLOO");
    
    // task_create("k1", 31, k_th1, 0);
    // process_execute(u_th1, "u1");
    // process_execute(u_th2, "u2");

    task_block_t *pcb = get_kpages(1);

    // interrupt_mask(INTERRUPT_KEYBOARD, true);
    interrupt_mask(INTERRUPT_TIMER, true);
    set_interrupt_state(true);

    while (1)
        ;
}
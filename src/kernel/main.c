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
extern void keyboard_init();

// void tmp1(void* arg) {
//     char* para = (char*) arg;
//     while (1) {
//         printk(para);
//     }
// }

// void tmp2(void* arg) {
//     char* para = (char*) arg;
//     while (1) {
//         printk(para);
//     }
// }

void kernel_main() {
    LOGK("Kernel Init...", MAGIC);
    interrupt_init();
    timer_init();
    task_init();
    keyboard_init();
    // task_create("k_thread_1", 31, tmp1, "YB");
    // task_create("k_thread_2", 8, tmp2, "HELLOO");

    interrupt_mask(INTERRUPT_KEYBOARD, true);
    set_interrupt_state(true);

    while (1)
        ;
}
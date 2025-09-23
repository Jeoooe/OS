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
#include <fs/fs.h>

extern void interrupt_init();
extern void timer_init();
extern void task_init();
extern void keyboard_init();
extern void syscall_init();
extern void ide_init();
extern void filesys_init();

extern void* sys_malloc(uint32_t size);
extern void sys_free(void* ptr);

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
    filesys_init();

    sys_open("/file1", O_CREAT);
    set_interrupt_state(true);
    

    while (1)
        ;
}
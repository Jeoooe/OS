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
#include <memory.h>

extern void interrupt_init();
extern void timer_init();
extern void task_init();
extern void keyboard_init();
extern void syscall_init();
extern void ide_init();
extern void filesys_init();


static void init();

void kernel_main() {
    LOGK("Kernel Init... %d", MAGIC);

    interrupt_init();
    timer_init();
    task_init();
    syscall_init();

    interrupt_mask(INTERRUPT_TIMER, true);
    set_interrupt_state(true);
    //进入用户模式
    asm(
        "movl %%esp, %%eax\n"
        "pushl $0x2b\n"
        "pushl %%eax\n"
        "pushfl\n"
        "pushl $0x23\n"
        "pushl $1f\n"
        "iret\n"
        "1:\t movl $0x2b, %%eax\n"
        "movl %%eax, %%ds\n"
        "movl %%eax, %%es\n"
        "movl %%eax, %%fs\n"
        :::"ax"  
    );

    init();

    while (1)
        ;

    
    

    
    interrupt_mask(INTERRUPT_SLAVE, true);
    interrupt_mask(INTERRUPT_HARDDISK_MASTER, true);
    interrupt_mask(INTERRUPT_HARDDISK_SLAVE, true);

   
    //从这里开始是用户态
    
    BMB;
    while (1) {
        asm volatile("sti;hlt");
        task_block(TASK_BLOCKED);
    }
        
    ide_init();
    keyboard_init();
}

void init() {
    printf("Init...\n");
}
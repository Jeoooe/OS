#include <os.h>
#include <interrupt.h>
#include <debug.h>
#include <thread.h>
#include <syscall.h>
#include <memory.h>

#include <stdio.h>

extern void interrupt_init();
extern void timer_init();
extern void task_init();
extern void keyboard_init();
extern void syscall_init();
extern void ide_init();
extern void device_init();
extern void buffer_init();
extern void root_setup();

static void init();


void kernel_main() {
    LOGK("Kernel Init... %d", MAGIC);

    interrupt_init();
    timer_init();
    syscall_init();

    //设备有关
    device_init();
    buffer_init();
    ide_init();
    // keyboard_init();

    //线程初始化
    task_init();

    interrupt_mask(INTERRUPT_TIMER, true);
    interrupt_mask(INTERRUPT_SLAVE, true);
    interrupt_mask(INTERRUPT_HARDDISK_MASTER, true);
    interrupt_mask(INTERRUPT_HARDDISK_SLAVE, true);
    set_interrupt_state(true);

    //文件系统
    root_setup();
    while (1);

    //进入用户模式
    asm(
        // "subl $0x300, %%esp\n"
        "movl %0, %%eax\n"
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
        ::"i"(USER_STACK_TOP):"ax"  
    );
    //从这里开始是用户态

    init();

    while (1)
        ;

    
    

    


   
    
    
}

void init() {
    BMB;
    int x = 0, y = 0;
    int pid = fork();

    if (pid == 0) { //子进程
        x = 100;
        printf("Kid\n");
        exit(x);
    }
    else {
        int status;
        waitpid(pid, &status, 0);
        printf("Father has %d, status: %d\n", pid, status);
    }

    while (1)
        ;
}
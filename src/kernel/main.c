#include <os.h>
#include <interrupt.h>
#include <debug.h>
#include <thread.h>
#include <syscall.h>
#include <fs/fcntl.h>

extern void interrupt_init();
extern void timer_init();
extern void task_init();
extern void syscall_init();

static void init();


void kernel_main() {
    LOGK("Kernel Init... %d", MAGIC);

    interrupt_init();
    timer_init();
    syscall_init();

    //线程初始化
    task_init();

    interrupt_mask(INTERRUPT_TIMER, true);
    interrupt_mask(INTERRUPT_KEYBOARD, true);
    interrupt_mask(INTERRUPT_SLAVE, true);
    interrupt_mask(INTERRUPT_HARDDISK_MASTER, true);
    interrupt_mask(INTERRUPT_HARDDISK_SLAVE, true);
    set_interrupt_state(true);

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

#include <device/dev.h>
static char getch() {
    char a;
    (void) read(0, &a, 1);
    return a;
}
static void putch(char a) {
    write(1, &a, 1);
}

void init() {
    setup();

    //标准输出输入错误
    (void) open("/dev/tty0", O_RDWR, 0);    //stdin
    (void) dup(0);                          //stdout
    (void) dup(0);                          //stderr

    int pid = fork();
    if (pid) { //父进程
        int status;
        int err = waitpid(pid, &status, 0);
        putch((char)status);
    } else {
        exit(1);
        execve("hello", NULL, NULL);
    }
    while (1)
        ;
}
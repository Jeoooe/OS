#include <signal.h>
#include <thread.h>
#include <interrupt.h>

typedef struct signal_frame_t {
    void (*sa_restorer)(void);
    uint32_t signr;
    uint32_t blocked;
    uint32_t eax;       //原系统调用返回值
    uint32_t ecx;
    uint32_t edx;
    uint32_t eflags;
    uint32_t old_eip;
} signal_frame_t;

extern void do_exit(int error_code);
extern void sa_restorer(void);

void send_sig(int signum, task_block_t *task) {
    if (signum >= 0 && signum <= 32)
        task->signals.pending |= 1 << (signum - 1);
}

signal_handler_t sys_signal(int signum, signal_handler_t handler) {
    //超出范围, 以及kill信号不允许修改
    if (signum < 1 || signum > 32 || signum == SIGKILL) 
        return (signal_handler_t)-1;
    task_block_t *current = running_task();
    signal_handler_t old_handler = current->signals.sa_handler[signum];
    current->signals.sa_handler[signum] = handler;
    if (handler == SIG_IGN)
        current->signals.blocked |= 1 << (uint32_t)signum;  //1为屏蔽
    return old_handler;
}


void do_signal() {
    task_block_t *cur = running_task();
    signal_table_t *signal = &cur->signals;
    interrupt_stack_t *intr_frame = (interrupt_stack_t*)(
        (uint32_t)cur + PAGE_SIZE - sizeof(interrupt_stack_t)
    );

    //找到第一个信号
    uint32_t pending = signal->pending & ~signal->blocked;
    uint32_t sig = 1;
    for (; sig <= 31; sig++) {
        if (pending & (1 << (sig - 1))) {
            break;
        }
    }
    if (sig > 31) return;   //没有信号
    signal->pending &= ~(1 << (sig - 1));
    signal_handler_t handler = cur->signals.sa_handler[sig];
    //不是默认, 即非0
    if (!handler) {
        if (sig == SIGCHLD) return;
        else {
            do_exit(1 << (sig - 1));
        }
    }
    //下面是信号句柄的调用
    cur->signals.sa_handler[sig] = SIG_DFL;
    uint32_t esp = intr_frame->esp;
    //修改用户栈顶, 插入信号处理栈帧
    signal_frame_t *frame = (signal_frame_t *)(esp - sizeof(signal_frame_t));
    frame->old_eip = (uint32_t)intr_frame->eip;
    frame->eflags = intr_frame->eflags;
    frame->edx = intr_frame->edx;
    frame->ecx = intr_frame->ecx;
    frame->eax = intr_frame->eax;
    frame->blocked = signal->blocked;
    frame->signr = sig;
    frame->sa_restorer = sa_restorer;
    //接下来修改中断返回的地址
    intr_frame->eip = handler;
    intr_frame->esp -= sizeof(signal_frame_t);
}
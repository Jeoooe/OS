#ifndef OS_THREAD_H
#define OS_THREAD_H

#include <stdint.h>

typedef void (*thread_func)(void*);

typedef enum task_status {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
} task_status;

//线程栈
typedef struct thread_stack_t {
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;
    
    //线程第一次执行时会指向待调用函数kernel_thread,
    //其余时候是switch_to的返回地址
    void (*eip)(thread_func func, void* func_arg);

    /* 第一次调度的时候才会使用
     * 会进入kernel_thread,然后取参数
    */
    void *unused; //占位符,充作返回地址
    thread_func function;
    void* func_arg;
} thread_stack_t;

typedef struct task_block_t {
    uint32_t* self_kstack;  //线程在内核态下运行时使用的栈
    task_status status;
    uint8_t priority;
    char name[16];
    uint32_t magic;
} task_block_t;


task_block_t* thread_start(char* name, int priority, thread_func function, void* func_arg);

#endif
/* 
 * 命名比较混乱, 这里任务和线程意义是一样的
 */

#ifndef OS_THREAD_H
#define OS_THREAD_H

#include <stdint.h>
#include <list.h>
#include <bitmap.h>
#include <memory.h>

typedef int16_t pid_t;
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
typedef struct task_stack_t {
    /* 保存上下文环节, 即以下四个寄存器 */
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;
    
    //线程第一次执行时会指向待调用函数kernel_thread,
    //其余时候是switch_to的返回地址
    void (*eip)(thread_func func, void* func_arg);

    /* 第一次调度的时候才会使用, 
     * 会进入kernel_thread,此时栈顶为以下三个变量,第一个参数是function
    */
    void *unused; //占位符,充作返回地址
    thread_func function;
    void* func_arg;
} task_stack_t;

//内核PCB
typedef struct task_block_t {
    uint32_t* self_kstack;  //线程在内核态下运行时使用的栈
    pid_t pid;
    task_status status;
    char name[16];
    uint8_t priority;
    uint8_t ticks;
    uint32_t elapsed_ticks;
    list_node_t node;
    uint32_t pd_addr;  //进程的页表地址
    vaddr_pool_t vaddr_pool;    //进程虚拟地址池
    memory_block_desc_t u_block_descs[MEMORY_DESC_CNT]; //用户进程的内存块描述符
    uint32_t magic;
} task_block_t;

//创建线程
task_block_t* task_create(char* name, int priority, thread_func function, void* func_arg);

//获取当前任务
task_block_t* running_task();

//任务调度
void schedule();

/* 任务阻塞相关 */
//阻塞自己
void task_block(task_status status);
//解除阻塞
void task_unblock(task_block_t* task);



#endif
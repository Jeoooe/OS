/* 
 * 命名比较混乱, 这里任务和线程意义是一样的
 */

#ifndef OS_THREAD_H
#define OS_THREAD_H

#include <stdint.h>
#include <list.h>
#include <bitmap.h>
#include <memory.h>

#define MAX_FILES_OPEN_PER_PROC 8

typedef void (*thread_func)(void);

typedef enum task_status {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
} task_status;

typedef enum task_user {
    UID_KERNEL,
    UID_USER
} task_user;

//线程栈
typedef struct task_stack_t {
    /* 保存上下文环节, 即以下四个寄存器 */
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;
    
    //switch_to的返回地址
    void (*eip)(thread_func func, void* func_arg);
} task_stack_t;

//内核PCB
typedef struct task_block_t {
    uint32_t* self_kstack;  //线程在内核态下运行时使用的栈
    uint32_t uid;
    pid_t pid;
    task_status status;
    char name[16];
    uint8_t priority;
    uint8_t ticks;
    uint32_t jiffies;
    list_node_t node;
    uint32_t pd_addr;  //进程的页表地址
    vaddr_pool_t vaddr_pool;    //进程虚拟地址池
    memory_block_desc_t u_block_descs[MEMORY_DESC_CNT]; //用户进程的内存块描述符
    uint32_t magic;
} task_block_t;

//创建线程
task_block_t* task_create(char* name, int priority, thread_func function);

//获取当前任务
task_block_t* running_task();

//任务调度
void schedule();

void task_yield();

/* 任务阻塞相关 */
//阻塞自己
void task_block(task_status status);
//解除阻塞
void task_unblock(task_block_t* task);



#endif
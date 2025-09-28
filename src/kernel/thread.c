#include <thread.h>
#include <memory.h>
#include <string.h>
#include <os.h>
#include <interrupt.h>
#include <assert.h>
#include <debug.h>
#include <mutex.h>
#include <stdio.h>

#define MAX_THREAD_COUNT 64

task_block_t* main_thread;  //主线程
task_block_t* idle_thread;  //空闲线程

task_block_t* all_threads[MAX_THREAD_COUNT];

lock_t pid_lock;

//获取一个空的任务
task_block_t* get_free_task() {
    lock_acquire(&pid_lock);
    for (int i = 2;i < MAX_THREAD_COUNT;i++) {
        if (all_threads[i] != NULL) 
            continue;
        all_threads[i] = get_kpages(1);
        memset(all_threads[i], 0, sizeof(task_block_t));
        all_threads[i]->pid = i;
        lock_release(&pid_lock);
        return all_threads[i];
    }
    lock_release(&pid_lock);
    return NULL;
}

task_block_t* running_task() {
    uint32_t esp;
    asm volatile("movl %%esp, %0" : "=g"(esp));
    return (task_block_t*)(esp & 0xfffff000);
}

//创建线程或进程后第一次调度会进入此函数 
// static void kernel_thread(thread_func function, void* func_arg) {
//     set_interrupt_state(true);  //开中断
//     function(func_arg);
// }

static void idle() {
    while(1) {
        asm volatile("sti\nhlt");
        task_block(TASK_BLOCKED);
    }
}

extern void switch_to(task_block_t* cur, task_block_t* next);
extern void process_activate(task_block_t *task);
void schedule() {
    assert(!get_interrupt_state()); //处于关中断状态
    task_block_t *cur = running_task();
    task_block_t *next;
    for (size_t i = 0;i < MAX_THREAD_COUNT;i++) {
        if (all_threads[i] == NULL) continue;
        if (all_threads[i] == cur) continue;
        if (all_threads[i]->status != TASK_READY) continue;

        if (next == NULL) {
            next = all_threads[i];
            continue;
        }
        //比较时间片
        if (all_threads[i]->jiffies < next->jiffies || all_threads[i]->ticks > next->ticks) {
            next = all_threads[i];
        }
    }
    if (cur->status == TASK_RUNNING) {
        //时间片到期
        cur->ticks = cur->priority;
        cur->status = TASK_READY;
    }
    else {
        /* 其他事件发生 , 不加入READY队列 */ 
    }
    // assert(!list_empty(&ready_task_list));
    if (!next) {
        task_unblock(idle_thread);
    }


    next->status = TASK_RUNNING;

    //激活任务的页目录并更新tss
    // process_activate(next);

    switch_to(cur, next);
}

void task_yield() {
    task_block_t* task = running_task();
    bool state = interrupt_disable();
    task->status = TASK_READY;
    schedule();
    set_interrupt_state(state);   
}



void task_block(task_status status) {
    bool intr_state = interrupt_disable();
    task_block_t* task = running_task();
    //正在运行的任务
    assert(task->status == TASK_RUNNING);
    //只能是阻塞为以下三个状态
    assert(status == TASK_BLOCKED || status == TASK_WAITING || status == TASK_HANGING);
    

    task->status = status;

    //主动调度
    schedule();

    set_interrupt_state(intr_state);
}

void task_unblock(task_block_t* task) {
    bool intr_state = interrupt_disable();
    assert(task->status == TASK_BLOCKED ||
        task->status == TASK_WAITING || task->status == TASK_HANGING);

    task->status = TASK_READY;

    set_interrupt_state(intr_state);
}

void task_block_init(task_block_t* task, char* name, int priority) {
    assert(task != NULL);
    /* PCB信息 */
    //清空PCB一页
    strcpy(task->name, name);
    task->uid = UID_KERNEL;
    task->status = TASK_READY;
    task->priority = priority;
    task->ticks = priority;
    task->jiffies = 0;
    task->pd_addr = 0;

    //799
    task->magic = MAGIC;
}

void task_stack_init(task_block_t* task, thread_func function) {
    task->self_kstack = (uint32_t*)((uint32_t)task + PAGE_SIZE);
    task->self_kstack -= sizeof(interrupt_stack_t);
    task->self_kstack -= sizeof(task_stack_t);
    task_stack_t* kstack = (task_stack_t*)task->self_kstack;
    kstack->eip = function;
    kstack->ebp = kstack->ebx = kstack->edi = kstack->esi = 0;
}

//创建一个线程
task_block_t* task_create(char* name,
    int priority, thread_func function
) {
    task_block_t *task = get_free_task();
    assert(task != NULL);
    task_block_init(task, name, priority);

    //预留栈空间
    task_stack_init(task, function);
    return task;
}

static void task_setup() {
    task_block_t* task = running_task();
    task->magic = MAGIC;
    task->ticks = 61;
    memset(all_threads, 0, sizeof(all_threads));
}

void task_a() {
    set_interrupt_state(true);
    printk("in task_a");
    while (1) ;
}
void task_b() {
    set_interrupt_state(true);
    printk("in task_b");
    while (1) ;
}

void task_init() {
    lock_init(&pid_lock);    
    task_setup();    

    // task_create("task_a", 31, task_a);
    // task_create("task_a", 31, task_b);
}
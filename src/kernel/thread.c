#include <thread.h>
#include <memory.h>
#include <string.h>
#include <os.h>
#include <interrupt.h>
#include <assert.h>

#define MAX_THREAD_COUNT 64

/* 全局变量 */
task_block_t* main_thread;  //主线程
list_t ready_task_list;

task_block_t* all_threads[MAX_THREAD_COUNT];

static task_block_t* get_free_task() {
    for (int i = 0;i < MAX_THREAD_COUNT;i++) {
        if (all_threads[i] != NULL) 
            continue;
        all_threads[i] = get_kpages(1);
        memset(all_threads[i], 0, sizeof(task_block_t));
        return all_threads[i];
    }
    return NULL;
}

task_block_t* running_task() {
    uint32_t esp;
    asm volatile("movl %%esp, %0" : "=g"(esp));
    return (task_block_t*)(esp & 0xfffff000);
}

extern void switch_to(task_block_t* cur, task_block_t* next);
void schedule() {
    assert(!get_interrupt_state()); //处于关中断状态
    task_block_t *cur = running_task();
    if (cur->status == TASK_RUNNING) {
        //时间片到期
        list_pushback(&ready_task_list, &cur->node);
        cur->ticks = cur->priority;
        cur->status = TASK_READY;
    }
    else {
        /* 其他事件发生 , 不加入READY队列 */ 
    }
    assert(!list_empty(&ready_task_list));
    list_node_t *next_node = list_pop(&ready_task_list);
    task_block_t *next = element_entry(task_block_t, node, next_node);
    next->status = TASK_RUNNING;
    switch_to(cur, next);
}

static void kernel_thread(thread_func function, void* func_arg) {
    set_interrupt_state(true);  //开中断
    function(func_arg);
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
    assert(!list_search(&ready_task_list, &task->node));

    task->status = TASK_READY;

    list_push(&ready_task_list, &task->node);

    set_interrupt_state(intr_state);
}



//创建一个线程
task_block_t* task_create(char* name,
    int priority, thread_func function, void* func_arg
) {
    task_block_t *task = get_free_task();
    assert(task != NULL);
    /* PCB信息 */
    //清空PCB一页
    strcpy(task->name, name);
    task->status = TASK_READY;
    task->priority = priority;
    task->ticks = priority;
    task->elapsed_ticks = 0;
    task->pde_addr = 0;
    task->magic = MAGIC;

    //预留栈空间
    task->self_kstack = (uint32_t*)((uint32_t)task + PAGE_SIZE);
    task->self_kstack -= sizeof(interrupt_stack_t);
    task->self_kstack -= sizeof(task_stack_t);
    task_stack_t* kstack = (task_stack_t*)task->self_kstack;
    kstack->eip = kernel_thread;
    kstack->function = function;
    kstack->func_arg = func_arg;
    kstack->ebp = kstack->ebx = kstack->edi = kstack->esi = 0;

    /* 加入队列 */
    assert(!list_search(&ready_task_list, &task->node));
    list_pushback(&ready_task_list, &task->node);
    return task;
}

static void task_setup() {
    main_thread = running_task();
    strcpy(main_thread->name, "main");
    main_thread->status = TASK_RUNNING;
    main_thread->priority = main_thread->ticks = 31;
    main_thread->elapsed_ticks = 0;
    main_thread->pde_addr = 0;
    main_thread->magic = MAGIC;
    main_thread->self_kstack = (uint32_t*)((uint32_t)main_thread + PAGE_SIZE);
    all_threads[1] = main_thread;   //设置第1个任务
}

void task_init() {
    /* 将主线程加入 */
    list_init(&ready_task_list);
    task_setup();    
}
#include <thread.h>
#include <memory.h>
#include <string.h>
#include <os.h>
#include <interrupt.h>
#include <assert.h>
#include <debug.h>
#include <mutex.h>
#include <stdio.h>
#include <tss.h>

#define get_cr3(n) asm("movl %%cr3, %%eax; movl %%eax, %0":"=r"(n))

extern bitmap_t kernel_vaddr_map; //from memory.c 内核虚拟内存位图

task_block_t* main_thread;  //主线程

task_block_t* all_threads[MAX_THREAD_COUNT];

lock_t pid_lock;

//获取一个空的任务
task_block_t* get_free_task() {
    lock_acquire(&pid_lock);
    for (int i = 0;i < MAX_THREAD_COUNT;i++) {
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

//根据pid获取任务
task_block_t* get_task_by_pid(pid_t pid) {
    return all_threads[pid];
}

//根据pid获取下标
size_t get_index_by_pid(pid_t pid) {
    return pid;
}

task_block_t* running_task() {
    uint32_t esp;
    asm volatile("movl %%esp, %0" : "=g"(esp));
    return (task_block_t*)(esp & 0xfffff000);
}

static void idle_thread() {
    ;
    while(1) {
        asm volatile("sti\nhlt");
        task_yield();
    }
}

extern void switch_to(task_block_t* cur, task_block_t* next);

//激活任务
static void process_activate(task_block_t *task) {
    assert(task->magic == MAGIC);
    uint32_t pde;
    get_cr3(pde);
    if (task->pd_addr != pde) {
        set_cr3(task->pd_addr);
    }
    if (task->uid != UID_KERNEL) {
        update_tss_esp(task);
    }
}

void schedule() {
    assert(!get_interrupt_state()); //处于关中断状态
    task_block_t *cur = running_task();
    task_block_t *next = NULL;
    for (size_t i = 1;i < MAX_THREAD_COUNT;i++) {
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

    //没有可以调度的了
    if (!next) {
        next = all_threads[0];
    }


    next->status = TASK_RUNNING;

    //激活任务的页目录并更新tss
    process_activate(next);
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
    assert(task->status == TASK_BLOCKED || task->status == TASK_WAITING || task->status == TASK_HANGING);

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

extern void intr_exit();

static void task_setup() {
    memset(all_threads, 0, sizeof(all_threads));
    /* Idle 进程 */
    task_block_t* task = get_free_task();
    memset(task, 0, PAGE_SIZE);
    task->uid = UID_KERNEL;
    task->status = TASK_BLOCKED;
    task->priority = 8;
    task->ticks = 8;
    task->jiffies = 0;
    task->brk = 0x100000;
    task->pd_addr = PDIR_BASE;
    task->self_kstack = (uint32_t*)((uint32_t)task + PAGE_SIZE);
    task->self_kstack -= sizeof(interrupt_stack_t);
    task->self_kstack -= sizeof(task_stack_t);
    task->magic = MAGIC;
    task_stack_t* kstack = (task_stack_t*)task->self_kstack;
    kstack->eip = intr_exit;
    kstack->ebp = kstack->ebx = kstack->edi = kstack->esi = 0;
    //构建中断栈, 需要定制一下, 因为貌似只有Idle进程是内核进程
    interrupt_stack_t* istack = (interrupt_stack_t*)((uint32_t)task->self_kstack + sizeof(task_stack_t));
    istack->eip = idle_thread;
    istack->ss = 0x10;
    istack->cs = 0x8;
    istack->esp = (uint32_t)task + PAGE_SIZE;

    /* Init 进程*/
    task = running_task();
    task->magic = MAGIC;
    task->uid = UID_USER;
    task->status = TASK_RUNNING;
    task->pid = 1;
    task->ppid = 0;
    task->priority = 31;
    task->brk = USER_BRK_INIT;
    task->ticks = task->priority;
    task->vaddr_map = &kernel_vaddr_map;
    task->pd_addr = PDIR_BASE;
    update_tss_esp(task);
    all_threads[1] = task;
}

void task_init() {
    lock_init(&pid_lock);    
    task_setup();    

    // task_create("task_a", 31, task_a);
    // task_create("task_a", 31, task_b);
}


extern void copy_page_table(task_block_t* to); //from memory.c

//系统调用fork
int sys_fork() {
    task_block_t* cur = running_task();
    task_block_t* child = get_free_task();
    uint32_t pid = child->pid;
    memcpy(child, cur, PAGE_SIZE);
    child->uid = UID_USER;
    child->pid = pid;
    child->ppid = cur->pid;
    child->status = TASK_READY;
    child->ticks = child->priority;

    //子进程内核栈
    //switch_to返回地址
    task_stack_t* stack = (task_stack_t*)(
        (uint32_t)child + PAGE_SIZE - 
        sizeof(interrupt_stack_t) - sizeof(task_stack_t)
    );
    stack->eip = intr_exit;
    //设置子进程内核栈位置
    child->self_kstack = (uint32_t*)stack;
    //中断返回值
    interrupt_stack_t* intr_stack = (interrupt_stack_t*)(
        (uint32_t)stack + sizeof(task_stack_t)
    );
    intr_stack->eax = 0;

    //复制虚拟内存位图
    child->vaddr_map = (bitmap_t*)kmalloc(sizeof(bitmap_t));
    uint8_t *buf = (uint8_t*)get_kpages(1); //只申请一页
    bitmap_init(child->vaddr_map, buf, PAGE_SIZE, cur->vaddr_map->offset);
    memcpy(buf, cur->vaddr_map->bits, PAGE_SIZE);

    //复制页目录及页表
    copy_page_table(child);

    //重置一下页表
    set_cr3(cur->pd_addr);

    
    return pid;
}
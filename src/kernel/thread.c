#include <thread.h>
#include <memory.h>
#include <string.h>
#include <os.h>
#include <interrupt.h>

static void kernel_thread(thread_func function, void* func_arg) {
    function(func_arg);
}

static inline void thread_init(task_block_t* block, char* name, int priority) {
    memset(block, 0, sizeof(task_block_t));
    strcpy(block->name, name);
    block->status = TASK_RUNNING;
    block->priority = priority;
    block->self_kstack = (uint32_t*)((uint32_t)block + PAGE_SIZE);
    block->magic = MAGIC;
}

static inline void thread_create(task_block_t* block, thread_func function, void *func_arg) {
    //预留栈空间
    block->self_kstack -= sizeof(interrupt_stack_t);
    block->self_kstack -= sizeof(thread_stack_t);
    thread_stack_t* kstack = (thread_stack_t*)block->self_kstack;
    kstack->eip = kernel_thread;
    kstack->function = function;
    kstack->func_arg = func_arg;
    kstack->ebp = kstack->ebx = kstack->edi = kstack->esi = 0;
}

task_block_t* thread_start(char* name,
    int priority, thread_func function, void* func_arg
) {
    task_block_t *block = get_kpages(1);
    thread_init(block, name, priority);
    thread_create(block, function, func_arg);

    //跳转过去线程
    asm volatile(
        "movl %0, %%esp\n"
        "pop %%ebp; pop %%ebx; pop %%edi; pop %%esi;\n"
        "ret"
        :: "g"(block->self_kstack) : "memory"
    );
    return block;
}

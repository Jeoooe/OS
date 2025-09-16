#include <interrupt.h>
#include <userprog.h>
#include <thread.h>
#include <global.h>
#include <memory.h>
#include <assert.h>
#include <tss.h>
#include <string.h>
#include <os.h>
#include <debug.h>

#define EFLAGS_MBS      (1 << 1)
#define EFLAGS_IF_1     (1 << 9)
#define EFLAGS_IF_0         0
#define EFLAGS_IOPL_3   (3 << 12)
#define EFLAGS_IOPL_0   (0 << 12)

#define DEFAULT_PRIO 31

extern void intr_exit(void);

/* 进入用户进程 */
void start_process(void* filename_) {
    void *function = filename_;
    task_block_t *task = running_task();
    task->self_kstack += sizeof(task_stack_t);
    interrupt_stack_t *proc_stack = (interrupt_stack_t*)task->self_kstack;
    
    //上下文
    proc_stack->edi = proc_stack->esi = proc_stack->ebp = proc_stack->esp_dummy = 0;
    proc_stack->ebx = proc_stack->edx = proc_stack->ecx = proc_stack->eax = 0;
    proc_stack->gs = 0;
    proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_U_DATA;
    proc_stack->eip = function;
    proc_stack->cs = SELECTOR_U_CODE;
    proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
    proc_stack->esp = (uint32_t)get_a_page(PF_USER, USER_STACK3_VADDR) + PAGE_SIZE;
    proc_stack->ss = SELECTOR_U_DATA;
    
    //进入用户进程
    asm volatile(
        "movl %0, %%esp\n"  //换内核栈
        "jmp intr_exit\n"
        :: "g"(proc_stack) : "memory"
    );
}


//激活进程的页表
void page_dir_activate(task_block_t *task) {
    //内核线程页表为0x100000
    //默认内核页目录
    uint32_t pagedir_paddr = PDIR_BASE;

    if (task->pd_addr != 0) {
        pagedir_paddr = addr_v2p(task->pd_addr);
    }
    // BMB;
    asm volatile("movl %0, %%cr3"::"r"(pagedir_paddr):"memory");
}

//激活线程或进程的页表并更新tss.esp0
void process_activate(task_block_t *task) {
    assert(task != NULL);

    page_dir_activate(task);

    //只有用户进程需要esp0
    if (task->pd_addr) {
        update_tss_esp(task);
    }
}


//复制页目录 返回页目录虚拟地址
uint32_t create_page_dir() {
    //页目录都是内核空间
    uint32_t *page_dir_vaddr = get_kpages(1);

    assert(page_dir_vaddr != NULL);

    //复制页表
    //第768项开始
    memcpy((uint32_t*)((uint32_t)page_dir_vaddr + 0x300 * 4), (uint32_t*)(0xfffff000 + 0x300 * 4), 1024);

    uint32_t new_pd_paddr = addr_v2p((uint32_t)page_dir_vaddr);
    page_dir_vaddr[1023] = new_pd_paddr | 0b111;
    return (uint32_t)page_dir_vaddr;
}

//用户进程的虚拟位图
void create_user_vaddr_bitmap(task_block_t *user_prog) {
    //该虚拟位图用于用户内存管理
    const uint32_t bitmap_bytes = (0xc0000000 - USER_VADDR_START) / PAGE_SIZE / 8;
    const uint32_t bitmap_pg_cnt = DIV_ROUND_UP(bitmap_bytes, PAGE_SIZE);
    uint8_t* bits = get_kpages(bitmap_pg_cnt);
    bitmap_init(&user_prog->vaddr_pool, bits, bitmap_bytes, USER_VADDR_START);
}

//创建用户进程
extern list_t ready_task_list;
extern void task_block_init(task_block_t* task, char* name, int priority);
extern void task_stack_init(task_block_t* task, thread_func function, void* func_arg);
extern task_block_t* get_free_task();
void process_execute(void *filename, char* name) {
    task_block_t* task = get_free_task();
    task_block_init(task, name, DEFAULT_PRIO);
    create_user_vaddr_bitmap(task);
    task_stack_init(task, start_process, filename);
    task->pd_addr = create_page_dir();

    bool state = interrupt_disable();
    list_pushback(&ready_task_list, &task->node);
    set_interrupt_state(state);
}
#ifndef OS_MEMORY_H
#define OS_MEMORY_H

#include <stdint.h>
#include <bitmap.h>
#include <list.h>

#define PAGE_SIZE 0x1000
#define PDIR_BASE 0x100000

#define USER_EXEC_START 0x1000000
#define USER_STACK_TOP 0x8000000
#define USER_STACK_SIZE 0x200000
#define USER_STACK_BOTTOM USER_STACK_TOP - USER_STACK_SIZE
#define USER_BRK_INIT USER_EXEC_START + 0x200000


typedef struct memory_block_desc_t {
    uint32_t block_size;
    uint32_t blocks_per_arena;
    list_t free_list;
} memory_block_desc_t;

typedef struct arena_t {
    memory_block_desc_t* desc;
    uint32_t large;
    uint32_t count;
    uint32_t magic;
} arena_t;



// void set_cr3(uint32_t pde);

//申请连续内存页 失败返回NULL
// void* get_page(pool_flag flag, uint32_t cnt);

//申请连续内核内存页 失败返回NULL 
//只会影响内核虚拟位图
void* get_kpages(uint32_t cnt);

//申请连续用户级内存页 失败返回NULL
// void* get_upages(uint32_t page_count);

//指定虚拟地址并映射一页
// void* get_a_page(pool_flag flag, uint32_t vaddr);

//释放vaddr起始pg_cnt个物理页框
void free_kpages(uint32_t vaddr, uint32_t cnt);

//虚拟地址转物理地址
// uint32_t addr_v2p(uint32_t vaddr);

//内存块描述符初始化
void block_desc_init(memory_block_desc_t* desc_array);


/* 系统调用 */
void* kmalloc(uint32_t size);
void kfree(void* ptr);

#endif
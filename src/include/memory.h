#ifndef OS_MEMORY_H
#define OS_MEMORY_H

#include <stdint.h>
#include <bitmap.h>
#include <list.h>

#define PAGE_SIZE 0x1000
#define PDIR_BASE 0x100000

#define MEMORY_DESC_CNT 7


typedef enum pool_flag {
    PF_KERNEL = 1,
    PF_USER
} pool_flag;

typedef struct vaddr_pool_t {
    bitmap_t bmap;
} vaddr_pool_t;

typedef struct memory_block_t {
    list_node_t node;
} memory_block_t;

typedef struct memory_block_desc_t {
    uint32_t block_size;
    uint32_t blocks_per_arena;
    list_t free_list;
} memory_block_desc_t;

void set_cr3(uint32_t pde);

//申请连续内存页 失败返回NULL
void* get_page(pool_flag flag, uint32_t cnt);

//申请连续内核内存页 失败返回NULL
void* get_kpages(uint32_t page_count);

//申请连续用户级内存页 失败返回NULL
void* get_upages(uint32_t page_count);

//指定虚拟地址并映射一页
void* get_a_page(pool_flag flag, uint32_t vaddr);

//释放vaddr起始pg_cnt个物理页框
void free_page(pool_flag pf, uint32_t vaddr, uint32_t pg_cnt);

//虚拟地址转物理地址
uint32_t addr_v2p(uint32_t vaddr);

//内存块描述符初始化
void block_desc_init(memory_block_desc_t* desc_array);


/* 系统调用 */
void* sys_malloc(uint32_t size);
void sys_free(void* ptr);

#endif
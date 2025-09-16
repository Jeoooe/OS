#ifndef OS_MEMORY_H
#define OS_MEMORY_H

#define PAGE_SIZE 0x1000

#define PDIR_BASE 0x100000

#include <stdint.h>

typedef enum pool_flag {
    PF_KERNEL = 1,
    PF_USER
} pool_flag;

void set_cr3(uint32_t pde);

//申请连续内存页 失败返回NULL
void* get_page(pool_flag flag, uint32_t cnt);

//申请连续内核内存页 失败返回NULL
void* get_kpages(uint32_t page_count);

//申请连续用户级内存页 失败返回NULL
void* get_upages(uint32_t page_count);

//指定虚拟地址并映射一页
void* get_a_page(pool_flag flag, uint32_t vaddr);

//虚拟地址转物理地址
uint32_t addr_v2p(uint32_t vaddr);

#endif
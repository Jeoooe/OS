#ifndef OS_MEMORY_H
#define OS_MEMORY_H

#define PAGE_SIZE 0x1000

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

#endif
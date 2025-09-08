#include <memory.h>
#include <stdint.h>
#include <string.h>
#include <debug.h>

#define PDE_BASE 0x100000
#define PAGE_SIZE 0x1000
#define INDEX_SIZE PAGE_SIZE / 4

#define INDEX_MASK(addr) (addr & 0xfffff000)
#define INDEX_TO_ADDR(index) (index << 12)

uint32_t *pde;

static void set_cr3(uint32_t pde) {
    asm volatile("movl %%eax, %%cr3"::"a"(pde));
}

static void page_init() {
    const uint32_t attr = 0b111;
    int i;
    uint32_t* pte1;
    pde = (uint32_t *)PDE_BASE;
    //清空页目录的内存
    memset(pde, 0, PAGE_SIZE);
    //设定首个页表和最后一个页表
    pde[0] = (PDE_BASE + PAGE_SIZE) | attr;
    pde[INDEX_SIZE - 1] = PDE_BASE | attr;

    //创建第一个页表
    pte1 = (uint32_t*)(PDE_BASE + PAGE_SIZE);
    for (i = 0; i < PDE_BASE / PAGE_SIZE;i++) {
        pte1[i] = INDEX_TO_ADDR(i) | attr;
    }

    //赋值cr3
    set_cr3((uint32_t)pde);
    //设置cr0
    BMB;
    asm volatile(
        "movl %cr0, %eax\n"
        "orl $0x80000000, %eax\n"
        "movl %eax, %cr0"
    );
}

//开启分页
void memory_init() {
    LOGK("Memory Init...\n");
    page_init();
}
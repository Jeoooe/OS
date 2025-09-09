#include <memory.h>
#include <stdint.h>
#include <string.h>
#include <debug.h>

#define PDE_BASE 0x100000
#define PAGE_SIZE 0x1000
#define INDEX_SIZE PAGE_SIZE / 4

#define INDEX_MASK(addr) (addr & 0xfffff000)
#define INDEX_TO_ADDR(index) (index << 12)

// #define KERNEL_MEMORY_SIZE 0x40000000   //内核内存大小 1GB
#define KERNEL_VADDR_START 0xc0000000   //内核内存起始虚拟地址

static void set_cr3(uint32_t pde) {
    asm volatile("movl %%eax, %%cr3"::"a"(pde));
}

static void page_init() {
    const uint32_t attr = 0b111;
    uint32_t *pde = (uint32_t *)PDE_BASE;
    uint32_t* pte;
    int i, addr;
    //清空页目录的内存
    memset(pde, 0, PAGE_SIZE);
    //设定首个页表和最后一个页表
    pde[0] = (PDE_BASE + PAGE_SIZE) | attr;
    pde[INDEX_SIZE - 1] = PDE_BASE | attr;

    //创建第一个页表
    pte = (uint32_t*)(PDE_BASE + PAGE_SIZE);
    for (i = 0; i < PDE_BASE / PAGE_SIZE;i++) {
        pte[i] = INDEX_TO_ADDR(i) | attr;
    }

    //映射内核内存空间
    //高1GB
    addr = (PDE_BASE + PAGE_SIZE) | attr;
    for (i = 0;i < 255;i++) {
        pde[i + 768] = addr;
        addr += 0x1000;
    }

    //赋值cr3
    set_cr3((uint32_t)pde);
    //设置cr0
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
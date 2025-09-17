#include <memory.h>
#include <stdint.h>
#include <string.h>
#include <debug.h>
#include <bitmap.h>
#include <os.h>
#include <assert.h>
#include <mutex.h>

#define INDEX_SIZE PAGE_SIZE / 4

#define MEMORY_BITMAP_BASE  0xc009a000
#define KERNEL_MEMORY_START 0xc0100000

#define INDEX_MASK(addr) (addr & 0xfffff000)
#define INDEX_TO_ADDR(index) (index << 12)

// #define KERNEL_MEMORY_SIZE 0x40000000   //内核内存大小 1GB
#define KERNEL_HEAP_START 0xc0100000   //内核堆内存起始虚拟地址

#define PDE_INDEX(vaddr) (vaddr >> 22)
#define PTE_INDEX(vaddr) ((vaddr >> 12) & 0x3ff)

typedef struct memory_pool_t {
    bitmap_t bmap;
    lock_t lock;
} memory_pool_t;

// bitmap_t kernel_pool, user_pool;         //物理内存池    
// bitmap_t kernel_vaddr_pool;              //虚拟地址池 为保证虚拟地址连续
// lock_t kernel_lock, user_lock;           //两个物理内存池对应的锁
memory_pool_t kernel_pool, user_pool;
vaddr_pool_t kernel_vaddr_pool;

static inline uint32_t* get_pde_ptr(uint32_t vaddr) {
    return (uint32_t *)(0xfffff000 | (PDE_INDEX(vaddr)<<2));
}

static inline uint32_t* get_pte_ptr(uint32_t vaddr) {
    return (uint32_t *)(0xffc00000 | ((PDE_INDEX(vaddr) << 12) | ((PTE_INDEX(vaddr) << 2))));
}

//获取连续虚拟地址
static uint32_t get_vaddr(bitmap_t *pool, uint32_t cnt) {
    uint32_t index = bitmap_scan(pool, cnt);
    int i;
    if (index == -1) {
        return 0;
    }
    for (i = 0;i < cnt;i++) {
        bitmap_set(pool, index + i, true);
    }
    return (pool->offset + index * PAGE_SIZE);
}

//从内存池中申请物理页返回页物理地址 失败返回0
static uint32_t get_phy_page(bitmap_t *pool) {
    uint32_t index = bitmap_scan(pool, 1);
    if (index == -1) {
        return 0;
    }
    bitmap_set(pool, index, true);
    return (pool->offset + index * PAGE_SIZE);
}

//映射虚拟地址和物理地址(物理地址必须是页开头)
static void link_vaddr(uint32_t vaddr, uint32_t paddr) {
    assert((paddr & 0xfff) == 0);
    //先获取页目录项
    uint32_t *pde = get_pde_ptr(vaddr);
    uint32_t *pte = get_pte_ptr(vaddr);
    if ((*pde & 1) == 0) {
        //该页表不存在
        uint32_t pte_page = get_phy_page(&kernel_pool.bmap);
        *pde = pte_page | 0b111;
        memset((void*)((int)pte & 0xfffff000), 0, PAGE_SIZE);
    }
    //再获取页表项
    
    //此页应该不存在
    assert(!((*pte) & 1));
    *pte = paddr | 0b111;
}

//申请连续虚拟页
void* get_page(pool_flag flag, uint32_t cnt) {
    vaddr_pool_t *vaddr_pool = (flag == PF_KERNEL) ? &kernel_vaddr_pool : &running_task()->vaddr_pool;
    memory_pool_t *phy_pool = (flag == PF_KERNEL) ? &kernel_pool : &user_pool;
    uint32_t vaddr_start;
    //获取连续虚拟地址
    vaddr_start = get_vaddr(&vaddr_pool->bmap, cnt);
    if (vaddr_start == 0) {
        return NULL;
    }
    //映射虚拟地址和物理地址
    uint32_t vaddr = vaddr_start, paddr;
    for (;cnt > 0;cnt--) {
        paddr = get_phy_page(&phy_pool->bmap);
        if (paddr == 0) {
            return NULL;
        }
        link_vaddr(vaddr, paddr);
        vaddr += PAGE_SIZE;
    }
    return (void *)vaddr_start;
}

//获取内存页
void* get_kpages(uint32_t page_count) {
    lock_acquire(&kernel_pool.lock);
    void* vaddr = get_page(PF_KERNEL, page_count);
    if (vaddr == NULL) return NULL;
    lock_release(&kernel_pool.lock);
    return vaddr;
}

void* get_upages(uint32_t page_count) {
    lock_acquire(&user_pool.lock);
    void* vaddr = get_page(PF_USER, page_count);
    if (vaddr == NULL) return NULL;
    lock_release(&user_pool.lock);
    return vaddr;
}

void* get_a_page(pool_flag flag, uint32_t vaddr) {
    memory_pool_t* mem_pool = (flag == PF_KERNEL) ? &kernel_pool : &user_pool;
    lock_t *lock = &mem_pool->lock;
    lock_acquire(lock);

    task_block_t *cur = running_task();
    int idx = -1;
    assert((cur->pd_addr != 0 && flag == PF_USER) ||
            (cur->pd_addr == 0 && flag == PF_KERNEL));
    
    if (flag == PF_USER) {
        idx = (vaddr - cur->vaddr_pool.bmap.offset) / PAGE_SIZE;
        bitmap_set(&cur->vaddr_pool.bmap, idx, true);
    }
    else {
        idx = (vaddr - kernel_pool.bmap.offset) / PAGE_SIZE;
        bitmap_set(&kernel_pool.bmap, idx, true);
    }
    
    uint32_t paddr = get_phy_page(&mem_pool->bmap);
    if (paddr == 0) 
        return NULL;
    
    link_vaddr(vaddr, paddr);


    lock_release(lock);
    return (void *)vaddr;
}


void set_cr3(uint32_t pde) {
    asm volatile("movl %%eax, %%cr3"::"a"(pde));
}

uint32_t addr_v2p(uint32_t vaddr) {
    uint32_t *pte = get_pte_ptr(vaddr);
    return ((*pte & 0xfffff000) + (vaddr & 0xfff));
}

// static void page_init() {
//     const uint32_t attr = 0b111;
//     uint32_t *pde = (uint32_t *)PDIR_BASE;
//     uint32_t* pte;
//     int i, addr;
//     //清空页目录的内存
//     memset(pde, 0, PAGE_SIZE);
//     //设定首个页表和最后一个页表
//     pde[0] = (PDIR_BASE + PAGE_SIZE) | attr;
//     pde[INDEX_SIZE - 1] = PDIR_BASE | attr;

//     //创建第一个页表
//     pte = (uint32_t*)(PDIR_BASE + PAGE_SIZE);
//     for (i = 0; i < PDIR_BASE / PAGE_SIZE;i++) {
//         pte[i] = INDEX_TO_ADDR(i) | attr;
//     }

//     //映射内核内存空间
//     //高1GB
//     addr = (PDIR_BASE + PAGE_SIZE) | attr;
//     for (i = 0;i < 255;i++) {
//         pde[i + 768] = addr;
//         addr += 0x1000;
//     }

//     //赋值cr3
//     set_cr3((uint32_t)pde);
//     //设置cr0
//     asm volatile(
//         "movl %cr0, %eax\n"
//         "orl $0x80000000, %eax\n"
//         "movl %eax, %cr0"
//     );
// }

static void memory_pool_init(uint32_t ards_addr) {
    LOGK("POOL Init...");
    /* 获取物理内存容量 */
    uint32_t phy_max_size = 0;
    const uint32_t ards_count = *(uint32_t *)ards_addr;
    struct ard_t {
        uint32_t base_low;
        uint32_t base_high;
        uint32_t length_low;
        uint32_t length_high;
        uint32_t type;
    } *ards = (struct ard_t *)(ards_addr + 4);
    
    for (int i = 0;i < ards_count;i++) {
        if (ards->length_low + ards->base_low > phy_max_size) {
            phy_max_size = ards->length_low + ards->base_low;
        }
        ards++;
    }
    LOGK("MEMORY SIZE: 0x%x", phy_max_size);
    
    /* 初始化内存池 */
    //内核占用高1G内存
    //内核256个页表
    const uint32_t page_table_size = PAGE_SIZE * 256;
    //加上低端占用1M
    const uint32_t used_memory = page_table_size + 0x100000;
    const uint32_t free_memory = phy_max_size - used_memory;
    const uint16_t free_pages = free_memory / PAGE_SIZE;

    //内核空闲页数,设定为总空闲的一半
    const uint16_t kernel_free_pages = free_pages / 2;
    const uint16_t user_free_pages = free_pages - kernel_free_pages;

    //内存池起始物理地址
    const uint32_t kernel_start_addr = used_memory;
    const uint32_t user_start_addr = kernel_start_addr + kernel_free_pages * PAGE_SIZE; 

    //内存池大小
    // const uint32_t kernel_size = kernel_free_pages * PAGE_SIZE;
    // const uint32_t user_size = user_free_pages * PAGE_SIZE;

    //位图大小
    const uint32_t kernel_bit_length = kernel_free_pages >> 3;
    const uint32_t user_bit_length = user_free_pages >> 3;

    //确认位图没有溢出
    assert(
        kernel_bit_length + user_bit_length + user_bit_length + MEMORY_BITMAP_BASE <= KERNEL_MEMORY_START
    );

    //确认位图偏移末三位是0
    assert((kernel_start_addr & 0xfff) == 0 && (user_start_addr & 0xfff) == 0);

    //初始化位图
    bitmap_init(&kernel_pool.bmap, 
        (uint8_t *)MEMORY_BITMAP_BASE, 
        kernel_bit_length, kernel_start_addr);
    bitmap_init(&user_pool.bmap  , 
        (uint8_t *)(MEMORY_BITMAP_BASE + kernel_bit_length), 
        user_bit_length, user_start_addr);

    //虚拟内存池
    bitmap_init(&kernel_vaddr_pool.bmap, 
        (uint8_t*)(MEMORY_BITMAP_BASE + kernel_bit_length + user_bit_length), 
        kernel_bit_length, KERNEL_HEAP_START);
    
}

//开启分页
void memory_init(uint32_t __, uint32_t ards_addr) {
    LOGK("Memory Init...");
    // page_init();
    memory_pool_init(ards_addr);
    lock_init(&kernel_pool.lock);
    lock_init(&user_pool.lock);
}
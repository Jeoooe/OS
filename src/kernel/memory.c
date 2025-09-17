#include <memory.h>
#include <stdint.h>
#include <string.h>
#include <debug.h>
#include <bitmap.h>
#include <os.h>
#include <assert.h>
#include <mutex.h>
#include <interrupt.h>

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
    uint32_t pool_size;
} memory_pool_t;

typedef struct arena_t {
    memory_block_desc_t* desc;
    /* large == true, cnt为页框数, 否则为空闲memory_block数 */
    uint32_t cnt;
    bool large; //是否大于1024字节
} arena_t;

memory_block_desc_t k_block_descs[MEMORY_DESC_CNT];

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

/* 内存申请
 */

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

/* 内存释放
*/

//释放物理地址paddr的页框
static void phy_free(uint32_t paddr) {
    assert((paddr & 0xfff) == 0);
    memory_pool_t* mem_pool;
    uint32_t bit_index = 0;
    if (paddr >= user_pool.bmap.offset) { //用户物理内存
        mem_pool = &user_pool;
    }
    else {  //内核物理内存
        mem_pool = &kernel_pool;
    }
    bit_index = (paddr - mem_pool->bmap.offset) / PAGE_SIZE;
    bitmap_set(&mem_pool->bmap, bit_index, false);
}

//去掉vaddr的映射, 仅去除pte
static void unlink_vaddr(uint32_t vaddr) {
    uint32_t *pte = get_pte_ptr(vaddr);
    *pte &= ~(1);
    asm volatile("invlpg %0"::"m"(vaddr):"memory");  //更新快表
}

//释放vaddr连续pg_cnt个虚拟页的地址
static void vaddr_remove(pool_flag pf, uint32_t vaddr, uint32_t pg_cnt) {
    uint32_t bit_index_start = 0, cnt = 0;
    vaddr_pool_t *v_pool;
    if (pf == PF_KERNEL) {
        v_pool = &kernel_vaddr_pool;
    }
    else {
        task_block_t* cur = running_task();
        v_pool = &cur->vaddr_pool;
    }
    bit_index_start = (vaddr - v_pool->bmap.offset) / PAGE_SIZE;
    for (;cnt < pg_cnt;cnt++) {
        bitmap_set(&v_pool->bmap, bit_index_start + cnt, false);
    }
}


void free_page(pool_flag pf, uint32_t vaddr, uint32_t pg_cnt) {
    uint32_t paddr, page_cnt = 0;
    assert(pg_cnt >= 1 && ((vaddr & 0xfff) == 0));
    paddr = addr_v2p(vaddr);

    //是页框并且在1M + 1k页目录 + 1k页表外
    assert((paddr & 0xfff) == 0 && paddr >= 0x102000);
    // memory_pool_t* mp;
    // if (paddr >= user_pool.bmap.offset) mp = &user_pool;
    // else mp = &kernel_pool;
    vaddr -= PAGE_SIZE;

    for (;page_cnt < pg_cnt; page_cnt++) {
        vaddr += PAGE_SIZE;
        paddr = addr_v2p(vaddr);
        phy_free(paddr);
        unlink_vaddr(vaddr);
    }
    vaddr_remove(pf, vaddr, pg_cnt);
}



void set_cr3(uint32_t pde) {
    asm volatile("movl %%eax, %%cr3"::"a"(pde));
}

uint32_t addr_v2p(uint32_t vaddr) {
    uint32_t *pte = get_pte_ptr(vaddr);
    return ((*pte & 0xfffff000) + (vaddr & 0xfff));
}


/* 初始化相关 */

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
    kernel_pool.pool_size = kernel_free_pages * PAGE_SIZE;
    user_pool.pool_size = user_free_pages * PAGE_SIZE;

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

void block_desc_init(memory_block_desc_t* desc_array) {
    uint16_t i, block_size = 16;    //字节数
    for (i = 0;i < MEMORY_DESC_CNT;i++) {
        desc_array[i].block_size = block_size;
        desc_array[i].blocks_per_arena = (PAGE_SIZE - sizeof(arena_t)) / block_size;

        list_init(&desc_array[i].free_list);
        block_size <<= 1;
    }
}

//开启分页
void memory_init(uint32_t __, uint32_t ards_addr) {
    LOGK("Memory Init...");
    // page_init();
    memory_pool_init(ards_addr);
    block_desc_init(k_block_descs);
    lock_init(&kernel_pool.lock);
    lock_init(&user_pool.lock);
}




/* 
 *  系统调用
 */

static memory_block_t* arena2block(arena_t* a, uint32_t idx) {
    return (memory_block_t*) ((uint32_t)a + sizeof(arena_t) + 
    idx * a->desc->block_size);
}

static arena_t* block2arena(memory_block_t* b) {
    return (arena_t *)((uint32_t)b & 0xfffff000);
}

void* sys_malloc(uint32_t size) {
    pool_flag pf;
    memory_pool_t *mem_pool;
    uint32_t pool_size;
    memory_block_desc_t *descs;
    task_block_t *task = running_task();

    if (task->pd_addr == 0) {
        //内核线程
        pf = PF_KERNEL;
        pool_size = kernel_pool.pool_size;
        mem_pool = &kernel_pool;
        descs = k_block_descs;
    } 
    else {
        pf = PF_USER;
        pool_size = user_pool.pool_size;
        mem_pool = &user_pool;
        descs = task->u_block_descs;
    }

    if (size < 0 || size > pool_size) {
        return NULL;
    }

    arena_t *a;
    memory_block_t *b;
    lock_t *lock = &mem_pool->lock;
    lock_acquire(lock);

    //大内存块
    if (size > 1024) {
        uint32_t page_cnt = DIV_ROUND_UP(size + sizeof(arena_t), PAGE_SIZE);
        a = get_page(pf, page_cnt);

        if (a == NULL) {
            lock_release(lock);
            return NULL;
        }

        a->desc = NULL;
        a->cnt = page_cnt;
        a->large = true;
        lock_release(lock);
        return (void*)(a + 1);
    }
    //小块内存
    int i;
    for (i = 0;i < MEMORY_DESC_CNT;i++) {
        if (size <= descs[i].block_size) break;
    }

    //空闲列表无了
    if (list_empty(&descs[i].free_list)) {
        a = get_page(pf, 1);
        if (a == NULL) {
            //没内存了
            lock_release(lock);
            return NULL;
        }
        a->desc = &descs[i];
        a->cnt = descs[i].blocks_per_arena;
        a->large = false;

        bool state = interrupt_disable();

        //新arena拆分内存块
        for (int j = 0;j < descs[i].blocks_per_arena;j++) {
            b = arena2block(a, j);
            list_pushback(&a->desc->free_list, &b->node);
        }
        set_interrupt_state(state);
    }
    list_node_t* block_node = list_pop(&descs[i].free_list);
    b = element_entry(memory_block_t, node, block_node);

    a = block2arena(b);
    a->cnt--;
    lock_release(lock);
    return (void*)b;
}


void sys_free(void* ptr) {
    assert(ptr != NULL);
    pool_flag pf;
    memory_pool_t* mem_pool;
    if (running_task()->pd_addr == 0) {
        assert((uint32_t)ptr >= KERNEL_HEAP_START);
        pf = PF_KERNEL;
        mem_pool = &kernel_pool;
    }
    else {
        pf = PF_USER;
        mem_pool = &user_pool;
    }

    lock_acquire(&mem_pool->lock);
    memory_block_t* b = ptr;
    arena_t* a = block2arena(b);

    if (a->desc == NULL && a->large == true) {
        //大于1024内存
        free_page(pf, (uint32_t)a, a->cnt);
    }
    else {
        list_pushback(&a->desc->free_list, &b->node);
        a->cnt++;
        if (a->cnt == a->desc->blocks_per_arena) {
            for (uint32_t i = 0;i < a->desc->blocks_per_arena;i++) {
                memory_block_t* blk = arena2block(a, i);
                list_remove(&blk->node);
            }
            free_page(pf, (uint32_t)a, 1);
        }
    }
    lock_release(&mem_pool->lock);
}
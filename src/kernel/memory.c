#include <os.h>
#include <memory.h>
#include <string.h>
#include <debug.h>
#include <stdint.h>
#include <bitmap.h>
#include <assert.h>
#include <thread.h>

// #define PAGE_SIZE 4096  //4kb

#define MAX_PHYSICAL_MEMORY 32 //支持512MB物理内存
#define PAGE_DIR_BASE 0x100000  //页目录地址
#define PAGE_TABLE_BASE (PAGE_DIR_BASE + 0x1000)  //第0个页表 
#define PAGE_TABLE_END (PAGE_DIR_BASE +  0x2000) //除去页表后可用地址开头
#define MAX_PHYSICAL_SIZE (MAX_PHYSICAL_MEMORY<<20) / PAGE_SIZE //最大页数
// #define MEM_MAP_END PAGE_TABLE_END + MEM_MAP_SIZE

//低1M的页数
#define LOW_1M (1<<20)
#define LOW_1M_PAGE_CNT LOW_1M / PAGE_SIZE

//主内存区起始地址
#define AVAI_MEM_START PAGE_TABLE_END
#define AVAI_MEM_INDEX 2
#define MEM_MAP_SIZE (MAX_PHYSICAL_SIZE - LOW_1M_PAGE_CNT)

//虚拟内存相关
#define VADDR_SIZE 0x20000             //虚拟内存位图字节数, 映射4GB
#define VADDR_MAP_BASE (0x9F000 - VADDR_SIZE)
#define VADDR_START 0x100000


//运算宏
//获取页目录项
#define PDE_VADDR(vaddr) (0xFFFFF000 | ((vaddr >> 20) & (~0b11)))
#define PTE_VADDR(vaddr) \
(0xFFC00000 | (vaddr >> 22) |((vaddr >> 10) & (~0b11)))


static uint8_t memory_map[MEM_MAP_SIZE];    //物理内存数组
bitmap_t vaddr_map;                  //虚拟内存位图
memory_block_desc_t block_desc[7];

//刷新快表
#define flush_tlb(vaddr) asm volatile("invlpg (%0)"::"r"(vaddr):"memory")

//空闲页
static uint32_t get_free_page() {
    //此函数可能有bug
    register uint32_t res = 0;
    asm(
        "movl %%ebx, %%ecx\n"
        "cld\n"
        "repnz scasb\n"
        "jnz 1f\n"
        "movb $1, -1(%%edi)\n"
        "subl %%ecx, %%ebx\n"
        "subl $1, %%ebx\n"
        "shll $12, %%ebx\n"
        "addl %2, %%ebx\n"
        "movl %%ebx, %0\n"
        "1:\n"
        :"=a"(res)
        :"0"(0),"i"(LOW_1M),"b"(MEM_MAP_SIZE),"D"(memory_map)
    );
    return res;
}

//映射一页
static void link_page(uint32_t vaddr, uint32_t paddr) {
    bitmap_t* vmap = &vaddr_map;
    uint32_t *pde = (uint32_t*)PDE_VADDR(vaddr);
    uint32_t *pte = (uint32_t*)PTE_VADDR(vaddr);
    //检测页目录项是否存在
    if (!(*pde & 1)) {
        uint32_t new_page_table = get_free_page();
        *pde = new_page_table | 0b111;
        memset(pte, 0, PAGE_SIZE);
    }
    *pte = paddr | 0b111;
    bitmap_set(vmap, (vaddr - vmap->offset) >> 12 , true);
    flush_tlb(vaddr);
}

static void unlink(uint32_t vaddr) {
    bitmap_t* vmap = &vaddr_map;
    // uint32_t *pde = (uint32_t*)PDE_VADDR(vaddr);
    uint32_t *pte = (uint32_t*)PTE_VADDR(vaddr);
    const uint32_t pindex = ((*pte) >> 12 )- LOW_1M_PAGE_CNT;
    
    memory_map[pindex]--;

    //没有再占用这页
    if (memory_map[pindex] == 0) {
        //暂时不知道干嘛
    }
    *pte = 0;
    bitmap_set(vmap, (vaddr - vmap->offset) >> 12, false);
    flush_tlb(vaddr);
}

//获取连续页
void* get_kpages(uint32_t cnt) {
    //获取虚拟地址    
    bitmap_t* vmap = &vaddr_map;
    const uint32_t bit_idx = bitmap_scan(vmap, cnt);
    if (bit_idx == -1) {
        return NULL;
    }
    const uint32_t vaddr_start = (bit_idx << 12) + vaddr_map.offset;
    uint32_t vaddr = vaddr_start;
    //获取物理页
    while (cnt-- >0) {
        uint32_t paddr = get_free_page();
        link_page(vaddr, paddr);
        vaddr += 0x1000;
    }
    return (void*)vaddr_start;
}

//释放连续页
void free_kpages(uint32_t vaddr, uint32_t cnt) {
    assert(vaddr >= LOW_1M);
    while (cnt-- > 0) {
        unlink(vaddr);
        vaddr += 0x1000;
    }
}

void memory_init(uint32_t __, uint32_t ards_addr) {
    // LOGK("可用内存的开头%d", MEM_MAP_END);
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

    //设定可用内存与不可用内存
    memset(memory_map, -1, AVAI_MEM_INDEX);
    memset((void*)((uint32_t)memory_map + AVAI_MEM_INDEX), 0, MEM_MAP_SIZE - AVAI_MEM_INDEX);

    bitmap_init(&vaddr_map, (uint8_t*)VADDR_MAP_BASE, VADDR_SIZE, VADDR_START);

    // 堆内存初始化
    uint32_t size = 16;
    int i = 0;
    for (;i < 7;i++) {
        block_desc[i].block_size = size;
        block_desc[i].blocks_per_arena = (PAGE_SIZE - sizeof(arena_t)) / size;
        list_init(&block_desc[i].free_list);
        size <<= 1;
    }
}


static inline void* arena2block(arena_t* a, size_t i) {
    return (void*)((uint32_t)a + i * a->desc->block_size + sizeof(arena_t));
}

static inline arena_t* block2arena(void* b) {
    return (arena_t*)(((uint32_t)b) & 0xfffff000);
}

void* kmalloc(uint32_t size) {
    arena_t* arena;
    
    if (size > 1024) {
        uint32_t count = DIV_ROUND_UP(size, PAGE_SIZE);
        arena = (arena_t*)get_kpages(count);
        arena->count = count;
        arena->large = 1;
        arena->desc = NULL;
        arena->magic = MAGIC;
        return (void*)((uint32_t)arena + sizeof(arena_t));
    }

    size_t i = 0;
    for (;i < 7;i++) {
        if (block_desc[i].block_size >= size) break;
    }

    if (list_empty(&block_desc[i].free_list)) {
        arena = (arena_t*)get_kpages(1);
        arena->count = block_desc[i].blocks_per_arena;
        arena->large = 0;
        arena->desc = &block_desc[i];
        arena->magic = MAGIC;
        for (int j = 0;j < block_desc[i].blocks_per_arena;j++) {
            list_node_t* b = arena2block(arena, j);
            list_push(&block_desc[i].free_list, b);
        }
    }

    void* b = (void*)list_pop(&block_desc[i].free_list);
    arena = block2arena(b);
    arena->count--;
    return b;
}

void kfree(void* ptr) {
    arena_t* arena = block2arena(ptr);

    if (arena->large == 1) {
        free_kpages((uint32_t)arena, arena->count);
        return;
    }

    list_pushback(&arena->desc->free_list, (list_node_t*)ptr);
    arena->count++;

    if (arena->desc->blocks_per_arena == arena->count) {
        for (int j = 0;j < arena->count;j++) {
            list_node_t* b = arena2block(arena, j);
            list_remove(b);
        }
        free_kpages((uint32_t)arena, 1);
    }
}
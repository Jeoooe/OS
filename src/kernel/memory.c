#include <os.h>
#include <memory.h>
#include <string.h>
#include <debug.h>
#include <stdint.h>
#include <bitmap.h>
#include <assert.h>
#include <thread.h>
#include <interrupt.h>
#include <fs/elf.h>

#ifndef likely
# define likely(x)   __builtin_expect(!!(x), 1)
# define unlikely(x) __builtin_expect(!!(x), 0)
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))

// #define PAGE_SIZE 4096  //4kb

#define MAX_PHYSICAL_MEMORY 32 //支持32MB物理内存
#define PAGE_DIR_BASE PDIR_BASE  //页目录地址
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
//内核虚拟内存 128MB 刚好一页
#define KERNEL_TOP 0x8000000
#define KERNEL_SHARED_PDE (KERNEL_TOP / 1024 / 4096)    //内核虚拟地址的最后一个页目录项
#define VADDR_SIZE 0x1000             
#define VADDR_MAP_BASE (0x9F000 - VADDR_SIZE)
#define VADDR_START 0x100000


//运算宏
//获取页目录项
#define PDE_VADDR(vaddr) (0xFFFFF000 | (((vaddr) >> 20) & (~0b11)))
#define PTE_VADDR(vaddr) (0xFFC00000 | (((vaddr) >> 10) & (~0b11)))

//物理地址转为物理地址数组索引
#define PADDR_TO_INDEX(paddr) (((paddr) >> 12) - LOW_1M_PAGE_CNT)

//虚拟地址转物理地址
#define vaddr_to_paddr(vaddr) (*(uint32_t*)PTE_VADDR(vaddr) & 0xFFFFF000)

static uint8_t memory_map[MEM_MAP_SIZE];    //物理内存数组
bitmap_t kernel_vaddr_map;                  //虚拟内存位图
memory_block_desc_t block_desc[7];
uint32_t max_physical_memory_size;


#define MEM_MAP_INDEX_LIMIT (max_physical_memory_size / PAGE_SIZE - LOW_1M_PAGE_CNT)
//刷新快表
#define flush_tlb(vaddr) asm volatile("invlpg (%0)"::"r"(vaddr):"memory")

//空闲页
//我想, 这里应该改为给内核分配物理页
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
        :"0"(0),"i"(LOW_1M),"b"(MEM_MAP_INDEX_LIMIT),"D"(memory_map)
    );
    return res;
}

//映射一页
static void link_page(bitmap_t* vmap, uint32_t vaddr, uint32_t paddr) {
    uint32_t *pde = (uint32_t*)PDE_VADDR(vaddr);
    uint32_t *pte = (uint32_t*)PTE_VADDR(vaddr);
    //检测页目录项是否存在
    if (!(*pde & 1)) {
        uint32_t new_page_table = get_free_page();
        *pde = new_page_table | 0b111;
        memset((void*)((uint32_t)pte & 0xFFFFF000), 0, PAGE_SIZE);
    }
    *pte = paddr | 0b111;
    bitmap_set(vmap, (vaddr - vmap->offset) >> 12 , true);
    flush_tlb(vaddr);
}

static void unlink(bitmap_t* vmap, uint32_t vaddr) {
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
    bitmap_t* vmap = &kernel_vaddr_map;
    const uint32_t bit_idx = bitmap_scan(vmap, cnt);
    if (bit_idx == -1) {
        return NULL;
    }
    const uint32_t vaddr_start = (bit_idx << 12) + kernel_vaddr_map.offset;
    uint32_t vaddr = vaddr_start;
    //获取物理页
    while (cnt-- >0) {
        uint32_t paddr = get_free_page();
        link_page(&kernel_vaddr_map, vaddr, paddr);
        vaddr += 0x1000;
    }
    return (void*)vaddr_start;
}

//释放连续页
void free_kpages(uint32_t vaddr, uint32_t cnt) {
    assert(vaddr >= LOW_1M);
    while (cnt-- > 0) {
        unlink(&kernel_vaddr_map, vaddr);
        vaddr += 0x1000;
    }
}

/*
 * 内存管理初始化
*/
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
    //全局变量
    max_physical_memory_size = phy_max_size;

    //设置0-0x1000不可用
    uint32_t *tmp_page_entry = (uint32_t*)0xFFC00000;
    *tmp_page_entry = 0;

    //设定可用内存与不可用内存
    memset(memory_map, -1, AVAI_MEM_INDEX);
    memset((void*)((uint32_t)memory_map + AVAI_MEM_INDEX), 0, MEM_MAP_SIZE - AVAI_MEM_INDEX);

    

    bitmap_init(&kernel_vaddr_map, (uint8_t*)VADDR_MAP_BASE, VADDR_SIZE, VADDR_START);

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


/*
    堆内存分配
*/

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

/*
    以下任务进程相关
*/


#define INDEX_TO_PGTABLE(index) (uint32_t*)(0xFFC00000 | (index << 12))
/*
    释放任务所占用的内存资源
    用于exit()
    此函数释放逻辑与写时复制有关联, 若改动写时复制逻辑可能需要同步改动此函数
    这里只是减少物理内存数组里的占用, 并不会改动页表和页目录的内容
*/

/// @brief  释放任务所占用的内存资源 用于exit()和execve.
///         此函数释放逻辑与写时复制有关联, 若改动写时复制逻辑可能需要同步改动此函数
///         这里只是减少物理内存数组里的占用, 并不会改动页表和页目录的内容
/// @param task 释放的任务
/// @param preserve_pd 是否保留页目录
void release_memory(task_block_t* task, bool preserve_pd) {
    assert(task != NULL);
    assert(!get_interrupt_state()); //关中断状态
    //事实上, 我好像并不需要清除资源, 只需要解映射就行了

    uint32_t *pd = (uint32_t*)0xFFFFF000;
    //只解除高于KERNEL_TOP的内存映射. 内核虚拟空间内的地址不需要释放
    for (int i = KERNEL_SHARED_PDE;i < 1023;i++) {   //遍历页目录项
        uint32_t *pde = &pd[i];
        if (!(*pde & 1))  //页目录项不存在
            continue;      
        uint32_t *pgtable = INDEX_TO_PGTABLE(i);
        for (int j = 0;j < 1024;j++) {   //遍历页表项
            uint32_t *pte = &pgtable[j];
            if (!(*pte & 1))    //页表项不存在 
                continue;
            uint32_t paddr = *pte & 0xFFFFF000;   //页框
            uint32_t mem_index = PADDR_TO_INDEX(paddr);
            memory_map[mem_index]--;
        }
        uint32_t pgtable_paddr = *pde & 0xFFFFF000;
        memory_map[PADDR_TO_INDEX(pgtable_paddr)]--;
    }
    //页目录也释放掉
    if (!preserve_pd) {
        uint32_t pd_paddr = task->pd_addr;
        memory_map[PADDR_TO_INDEX(pd_paddr)]--; 
    }
}


/*  
    给任务复制页表, 用于fork()
    要在exit的时候释放
*/
void copy_page_table(task_block_t* to) {
    uint32_t *new_page_dir = (uint32_t*)get_kpages(1);
    uint32_t *cur_page_dir = (uint32_t*)(0xFFFFF000);
    //设置最后一项为自己
    uint32_t paddr = vaddr_to_paddr((uint32_t)new_page_dir);
    new_page_dir[1023] = paddr | 0b111;
    //在内核虚拟地址范围内不共享
    //即 [0, KERNEL_TOP]
    //刚好是 KERNEL_SHARED_PDE 内
    //改完之后可能存在问题, 即0x0地址可能会存在?
    memcpy(new_page_dir, cur_page_dir, KERNEL_SHARED_PDE * 4);  //复制内核虚拟地址的页目录项
    for (int i = KERNEL_SHARED_PDE;i < 1023;i++) {   //遍历页目录项
        uint32_t* pde = &cur_page_dir[i];
        if (*pde == 0) continue;
        //存在页目录项
        uint32_t *page_table = (uint32_t*)(0xFFC00000 | (i << 12));
        //设置只读, 物理内存引用+1
        //这里只是设置物理映射
        for (int j = 0;j < 1024;j++) {   //遍历页表项
            uint32_t *pte = &page_table[j];
            if (*pte == 0) continue;
            *pte &= ~(0b10);    //置为只读
            memory_map[PADDR_TO_INDEX(*pte & 0xFFFFF000)]++;
        }
        //这里开始复制整个页表的内容
        //复制页表
        uint32_t paddr = get_free_page();
        //利用第0页
        uint32_t *tmp_page_entry = (uint32_t*)0xFFC00000;
        *tmp_page_entry = paddr | 0b111;
        flush_tlb(0);
        memcpy((void*)0, page_table, PAGE_SIZE);
        *tmp_page_entry = 0;
        flush_tlb(0);
        //设置页目录项
        new_page_dir[i] = paddr | 0b111;
    }
    to->pd_addr = vaddr_to_paddr((uint32_t)new_page_dir);
}

static inline uint32_t get_cr2() {
    register uint32_t res;
    asm("movl %%cr2, %0":"=r"(res));
    return res;
}

//写时复制实现
//目前仅共享物理页框, 因此只复制页框
static void copy_on_write(task_block_t* cur_task, uint32_t vaddr) {
    vaddr &= 0xFFFFF000;    //需要是页开头

    uint32_t paddr = vaddr_to_paddr(vaddr);
    uint32_t index = PADDR_TO_INDEX(paddr);
    uint32_t *pte = (uint32_t*)PTE_VADDR(vaddr);

    assert(memory_map[index] > 0);
    assert(!get_interrupt_state()); //最好是关中断吧

    if (memory_map[index] > 1) {   //如果有任务在共享
        uint32_t paddr =get_free_page();
        uint32_t *tmp_pte = (uint32_t*)0xFFC00000;
        //然后复制一个页框
        *tmp_pte = paddr | 7;
        flush_tlb(0);
        memcpy((void*)0, (void*)vaddr, PAGE_SIZE);
        *tmp_pte = 0;   //恢复0为空
        flush_tlb(0);
        *pte = paddr | 7;
        memory_map[index]--;
    }
    *pte |= 0b10;   //RW位置1
}

//给当前进程 指定虚拟地址 映射一个物理页
void get_empty_page(uint32_t vaddr) {
    uint32_t new_page = get_free_page();
    //这里先这样写, 因为默认的虚拟地址池只能映射低0x8000000位. 
    //后续可能考虑去掉这个虚拟地址池, 好像没有什么用
    //25.12.31 还是有用的, 刚好是内核专用虚拟地址的映射
    if (vaddr < KERNEL_TOP)
        // link_page(running_task()->vaddr_map, vaddr & 0xFFFFF000, new_page);
        link_page(&kernel_vaddr_map, vaddr & 0xFFFFF000, new_page);
    else {
        uint32_t *pde = (uint32_t*)PDE_VADDR(vaddr);
        uint32_t *pte = (uint32_t*)PTE_VADDR(vaddr);
        //检测页目录项是否存在
        if (!(*pde & 1)) {
            uint32_t new_page_table = get_free_page();
            *pde = new_page_table | 0b111;
            memset((void*)((uint32_t)pte & 0xFFFFF000), 0, PAGE_SIZE);
        }
        *pte = new_page | 0b111;
    }
}

/// @brief 清除当前进程页目录 (只清除用户内存)
void free_page_directory() {
    memset((void *)PAGE_DIRECTORY_VADDR + KERNEL_SHARED_PDE, 
    0, PAGE_SIZE - KERNEL_SHARED_PDE * 4);
}


/*
    以下是按需加载部分
*/

//按需加载的部分, 如果出错可能要exit之类的


/// @brief 按需加载vaddr地址的程序段, 会检查是否合法
/// 
///  在`page_fault()`调用
/// @param vaddr 加载地址
void load_segment(uint32_t vaddr) {
    task_block_t *cur = running_task();
    Elf32_Phdr *phdr_array = (Elf32_Phdr *)cur->exec_phdr_list.array, *p;
    uint32_t i = 0;
    for (;i < cur->exec_phdr_list.length;i++) {
        p = &phdr_array[i];
        if (p->p_vaddr <= vaddr && vaddr < p->p_vaddr + p->p_memsz)
        break;
    }
    if (i == cur->exec_phdr_list.length) return;
    assert(p->p_align == PAGE_SIZE);   //这里要求加载段必须是页对齐
    vaddr &= 0xFFFFF000;    //页对齐
    //下面加载程序
    //这里计算要加载多少块. 一次性最多加载一页
    i = vaddr - p->p_vaddr;
    //计算要从文件里读取多少字节
    uint32_t bytes = i > p->p_filesz ? 0 : p->p_filesz - i;
    i = vaddr;  //保存本页开始位置
    get_empty_page(vaddr);
    while (bytes && vaddr - i < PAGE_SIZE) {
        //这里是计算vaddr位置对应文件中的偏移
        uint32_t block = (vaddr - p->p_vaddr + p->p_offset) / BLOCK_SIZE;
        block = get_block(cur->exe_file, block);
        assert(block);
        buffer_t *bh = bread(cur->exe_file->dev, block);
        assert(bh);
        //假如bytes不足一个块
        if (bytes < BLOCK_SIZE) {
            memcpy((void *)vaddr, bh->data, bytes);
            vaddr += bytes;
            bytes = 0;
            brelse(bh);
            break;
        } else {
            //bytes超过一个块
            memcpy((void *)vaddr, bh->data, BLOCK_SIZE);
        }
        vaddr += BLOCK_SIZE;
        bytes -= BLOCK_SIZE;
        brelse(bh);
    }
    //然后判断是否还需要填0, 因为内存大小可能大于文件大小
    if (vaddr - i < PAGE_SIZE && p->p_memsz > vaddr - p->p_vaddr) {
        memset((void *)vaddr, 0, p->p_memsz - (vaddr - p->p_vaddr));
    }

    //这里要判断flag, 设置页表项
    //主要是设置读写位
    uint32_t *pte = (uint32_t *)PTE_VADDR(i);
    if (p->p_flags & PF_W) *pte |= 0b10;    //可写
    else *pte &= ~0b10;                     //只读
} 

// 缺页异常
void page_fault(uint8_t vector, 
    uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, 
    uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax, 
    uint32_t gs, uint32_t fs, uint32_t es, uint32_t ds, 
    uint32_t error_code
) {
    uint32_t vaddr = get_cr2() & 0xfffff000;
    task_block_t* task = running_task();
    // assert(vaddr > 0x1000); //不在开头一页

    if (vaddr <= 0x1000) {
        printk("Try to access 0x%x\n", vaddr);
        panic("");
    }

    //特殊情况
    if (vaddr < USER_EXEC_START) {
        panic("Page fault in kernel page");
    }


    //页不存在
    if ((error_code & 1) == 0) {
        if (!(vaddr >= USER_STACK_BOTTOM || vaddr <= task->brk)) {
            panic("Page fault out of user range");
        }
        //申请一页
        if (!task->exe_file) {
            get_empty_page(vaddr);
            return;
        }
        //用户执行文件时候不存在页, 下面进行按需加载
        //TODO
        load_segment(vaddr);
    }

    //写访问异常
    if ((error_code & 2)) {    
        //TODO 确实是只读页而非共享页, 因为可执行文件存在只读段, 比如.text
        //写时复制
        copy_on_write(task, vaddr);
        return;
    }
}
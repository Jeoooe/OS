#include <stdint.h>
#include <global.h>
#include <thread.h>
#include <memory.h>
#include <debug.h>
#include <string.h>

#define TSS_ATTR_HIGH 0b10000000
#define TSS_ATTR_LOW 0b10001001

typedef struct tss_t {          
    uint32_t backlink; 
    uint32_t esp0; 
    uint32_t ss0; 
    uint32_t esp1; 
    uint32_t ss1; 
    uint32_t esp2; 
    uint32_t ss2; 
    uint32_t cr3; 
    uint32_t (*eip) (void); 
    uint32_t eflags; 
    uint32_t eax; 
    uint32_t ecx; 
    uint32_t edx; 
    uint32_t ebx; 
    uint32_t esp; 
    uint32_t ebp; 
    uint32_t esi; 
    uint32_t edi; 
    uint32_t es; 
    uint32_t cs; 
    uint32_t ss; 
    uint32_t ds; 
    uint32_t fs; 
    uint32_t gs; 
    uint32_t ldt; 
    uint32_t trace; 
    uint32_t io_base; 
} tss_t; 

//一个全局使用的tss段
static tss_t tss;

//tss中esp0字段
void update_tss_esp(task_block_t* task) {
    tss.esp0 = (uint32_t)task + PAGE_SIZE;
}

static void make_gdt_desc(gdt_desc_t* desc, uint32_t desc_addr, uint32_t limit, uint8_t attr_low, uint8_t attr_high) {
    desc->limit_low_word = limit & 0xffff;
    desc->base_low_word = desc_addr & 0xffff;
    desc->base_mid_byte = ((desc_addr >> 16) & 0xff);
    desc->base_high_byte = desc_addr >> 24;
    desc->attr_low_byte = (uint8_t)attr_low;
    desc->limit_high_attr_high = ((limit >> 16) & 0xf) | ((uint8_t)attr_high);
}

//设置tss段, 用户代码段和数据段
void tss_init() {
    LOGK("TSS Init...");
    
    //描述符指针
    descriptor_ptr gdt_ptr;

    //获取gdt基地址
    asm(
        "sgdt %0" ::"m"(gdt_ptr)
    );
    uint32_t gdt_base = gdt_ptr.base;
    uint32_t tss_size = sizeof(tss);
    memset(&tss, 0, tss_size);
    tss.ss0 = SELECTOR_K_DATA;
    tss.io_base = tss_size;

    //第三个描述符
    gdt_desc_t* desc = (gdt_desc_t*)(gdt_base + 0x18);
    make_gdt_desc(desc, (uint32_t)&tss, tss_size - 1, TSS_ATTR_LOW, TSS_ATTR_HIGH);

    //用户数据段和代码段
    desc++;
    make_gdt_desc(desc, 0, 0xfffff, GDT_CODE_ATTR_LOW_DPL3, GDT_ATTR_HIGH);
    desc++;
    make_gdt_desc(desc, 0, 0xfffff, GDT_DAYA_ATTR_LOW_DPL3, GDT_ATTR_HIGH);

    
    //六个描述符
    gdt_ptr.limit = (8 * 6) - 1;
    asm volatile("lgdt %0" :: "m"(gdt_ptr));
    asm volatile("ltr %w0" :: "r"(SELECTOR_K_TSS));
}